﻿#include "PacketManager.h"

#include "../Events/EventSystem.h"
#include "CorePacketComponents/DataReplicationPacketComponent.hpp"
#include "CorePacketComponents/ReturnAckComponent.hpp"
#include "CorePacketComponents/ServerDisconnectPacketComponent.hpp"
#include "CorePacketComponents/ServerPingPacketComponent.hpp"
#include "CorePacketComponents/SuccessfullyConnectedToServer.hpp"
#include "CorePacketComponents/ServerConnectPacketComponent.hpp"

namespace Net
{
PacketManager* PacketManager::instance_ = nullptr;

PacketManager::PacketManager()
{
    lastTimePacketSent_ = std::chrono::steady_clock::now();
}

PacketManager::~PacketManager()
{
    
}

PacketManager* PacketManager::Initialize()
{
    if (instance_ == nullptr)
    {
        using namespace std::chrono;
        
        instance_ = new PacketManager();

        instance_->lastUpdateTime_ = steady_clock::now();
        instance_->lastUpdateFinishedTime_ = steady_clock::now();
        
        instance_->RegisterDefaultPacketComponents();
    }
    
    return instance_;
}

PacketManager* PacketManager::Get()
{
    return instance_;
}

void PacketManager::End()
{
    if (instance_ != nullptr)
    {
        delete instance_;
        instance_ = nullptr;
    }
}

void PacketManager::Update()
{
    if (!SimpleNetLibCore::Get()->GetNetHandler()->IsRunning())
        return;
    
    using namespace std::chrono;

    const steady_clock::time_point currentTime = steady_clock::now();
    const duration<double> deltaTime = duration_cast<duration<double>>(currentTime - lastUpdateTime_);
    lastUpdateTime_ = currentTime;

    updateLag_ += deltaTime.count();

    if (updateLag_ >= FIXED_UPDATE_DELTA_TIME)
    {
        const duration<float> deltaTimeSinceLastFinishedUpdate = duration_cast<duration<float>>(currentTime - lastUpdateFinishedTime_);
        lastDeltaTime_ = deltaTimeSinceLastFinishedUpdate.count();
        
        FixedUpdate();
        
        updateLag_ -= FIXED_UPDATE_DELTA_TIME;
        lastUpdateFinishedTime_ = steady_clock::now();
    }
}

const PacketComponentAssociatedData* PacketManager::FetchPacketComponentAssociatedData(const uint16_t InIdentifier)
{
    if (packetAssociatedData_.find(InIdentifier) != packetAssociatedData_.end())
    {
        return &packetAssociatedData_.at(InIdentifier);
    }
    return nullptr;
}

void PacketManager::UpdateClientNetPosition(const sockaddr_storage& InAddress, const NetUtility::NetVector3& InPosition)
{
    SimpleNetLibCore::Get()->GetNetHandler()->GetNetConnectionHandler().UpdateNetCullingPosition(InAddress, InPosition);
}

void PacketManager::HandleComponent(const sockaddr_storage& InComponentSender, const PacketComponent& InPacketComponent)
{
    packetComponentHandleDelegator_.HandleComponent(InComponentSender, InPacketComponent); 
}

void PacketManager::FixedUpdate()
{
    if (managerType_ == ENetworkHandleType::None)
        return;
    
    if (!SimpleNetLibCore::Get()->GetNetHandler()->IsServer())
    {
        UpdateServerPinging();
    }
    
    for (std::pair<const sockaddr_storage, PacketTargetData>& packetTargetPair : packetTargetDataMap_)
    {
        const sockaddr_storage& target = packetTargetPair.first;
        PacketTargetData& targetData = packetTargetPair.second;

        UpdatePacketsWaitingForReturnAck(target, targetData);
        UpdatePacketsToSend(target, targetData);
    }

    ++updateIterator_;
    if (updateIterator_ > UPDATE_ITERATOR_RESET_NUMBER)
    {
        updateIterator_ = 0;
    }
}

void PacketManager::UpdatePacketsWaitingForReturnAck(const sockaddr_storage& InTarget, const PacketTargetData& InTargetData) const
{
    const std::map<uint32_t, std::pair<PacketFrequencyData, Packet>>& packetsNotReturned = InTargetData.GetPacketsNotReturned();
    for (const auto& packetDataPair : packetsNotReturned)
    {
        const PacketFrequencyData& frequencyData = packetDataPair.second.first;
        if (DoesUpdateIterMatchPacketFrequency(frequencyData, true))
        {
            const Packet& packet = packetDataPair.second.second;
            SimpleNetLibCore::Get()->GetNetHandler()->SendPacketToTarget(InTarget, packet);
        }
    }
}

void PacketManager::UpdatePacketsToSend(const sockaddr_storage& InTarget, PacketTargetData& InTargetData) const
{
    std::map<PacketFrequencyData, PacketToSendData>& packetComponents = InTargetData.GetPacketComponentsToSend();
    std::vector<PacketFrequencyData> toRemove;
    for (auto packetIter = packetComponents.begin(); packetIter != packetComponents.end(); ++packetIter)
    {
        const PacketFrequencyData& frequencyData = packetIter->first;
        if (DoesUpdateIterMatchPacketFrequency(frequencyData, false))
        {
            const EPacketHandlingType handlingType = frequencyData.handlingType;
            Packet packet = Packet(handlingType);
            
            PacketToSendData& packetSendData = packetIter->second;
            const std::vector<std::shared_ptr<PacketComponent>>& components = packetSendData.GetComponents();
            
            // Add Components to new Packet
            int componentsAdded = 0;
            if (!components.empty())
            {
                for (auto it = components.rbegin(); it != components.rend(); ++it)
                {
                    const EAddComponentResult result = packet.AddComponent(**it);
                    if (result != EAddComponentResult::Success)
                    {
                        break;
                    }

                    ++componentsAdded;
                }
            }

            // Update checksum
            packet.CalculateAndUpdateCheckSum();
            
            // Remove added components
            if (componentsAdded > 0)
            {
                packetSendData.RemoveComponents(static_cast<int>(components.size()) - componentsAdded, componentsAdded);
            }

            // Add to ack container if of Ack type
            if (handlingType == EPacketHandlingType::Ack)
            {
                InTargetData.AddAckPacketIfContainingData(frequencyData, packet);
            }
            
            if (components.empty())
            {
                toRemove.push_back(frequencyData);
            }
                
            SimpleNetLibCore::Get()->GetNetHandler()->SendPacketToTarget(InTarget, packet);
        }
    }

    // Remove empty frequency containers
    for (const PacketFrequencyData& dataToRemove : toRemove)
    {
        packetComponents.erase(dataToRemove);
    }
}

bool PacketManager::DoesUpdateIterMatchPacketFrequency(const PacketFrequencyData& InPacketFrequencyData, const bool InIsPacketResend) const
{
    if (InPacketFrequencyData.frequency == 0)
    {
        return false;
    }

    int modifiedFrequency = InPacketFrequencyData.frequency;
    if (InIsPacketResend)
    {
       modifiedFrequency = static_cast<int>(static_cast<float>(modifiedFrequency) * InPacketFrequencyData.ackFrequencyMultiplier);
    }
    
    const int updateIterator = std::abs(updateIterator_);
    return updateIterator % modifiedFrequency == 0;
}

void PacketManager::OnNetTargetConnected(const sockaddr_storage& InTarget)
{
    Get()->packetTargetDataMap_.insert({ InTarget, PacketTargetData() });
    EventSystem::Get()->onClientConnectEvent.Execute(InTarget);
}

void PacketManager::OnNetTargetDisconnection(const sockaddr_storage& InTarget, const ENetDisconnectType InDisconnectType)
{
    auto& packetTargetDataMap = Get()->packetTargetDataMap_;
    if (packetTargetDataMap.find(InTarget) != packetTargetDataMap.end())
    {
        packetTargetDataMap.erase(InTarget);
    }
    
    EventSystem::Get()->onClientDisconnectEvent.Execute(InTarget, InDisconnectType);
}

void PacketManager::OnAckReturnReceived(const sockaddr_storage& InTarget, const PacketComponent& InComponent)
{
    const ReturnAckComponent* ackComponent = static_cast<const ReturnAckComponent*>(&InComponent);
    if (packetTargetDataMap_.find(InTarget) != packetTargetDataMap_.end())
    {
        packetTargetDataMap_.at(InTarget).RemoveReturnedPacket(ackComponent->ackIdentifier);
    }
}

void PacketManager::RegisterDefaultPacketComponents()
{
    {
        const PacketComponentAssociatedData associatedData = PacketComponentAssociatedData
        {
            true,
            FIXED_UPDATE_DELTA_TIME,
            1.f,
        };
        RegisterPacketComponent<DataReplicationPacketComponent>(associatedData);
    }
    {
        const PacketComponentAssociatedData associatedData = PacketComponentAssociatedData
        {
            false,
            0.3f,
            4.f,
            EPacketHandlingType::Ack
        };
        RegisterPacketComponent<ServerConnectPacketComponent, NetHandler>(associatedData, &NetHandler::OnChildConnectionReceived, SimpleNetLibCore::Get()->GetNetHandler());
    }
    {
        const PacketComponentAssociatedData associatedData = PacketComponentAssociatedData
        {
            false,
            0.3f,
            4.f,
            EPacketHandlingType::Ack
        };
        RegisterPacketComponent<ServerDisconnectPacketComponent, NetHandler>(associatedData, &NetHandler::OnChildDisconnectReceived, SimpleNetLibCore::Get()->GetNetHandler());
    }
    {
        const PacketComponentAssociatedData associatedData = PacketComponentAssociatedData
        {
            false,
            0.05f,
            1.f,
            EPacketHandlingType::None
        };
        RegisterPacketComponent<ReturnAckComponent, PacketManager>(associatedData, &PacketManager::OnAckReturnReceived, this);
    }
    {
        const PacketComponentAssociatedData associatedData = PacketComponentAssociatedData
        {
            false,
            1.f,
            1.f,
            EPacketHandlingType::Ack
        };
        RegisterPacketComponent<ServerPingPacketComponent>(associatedData);
    }
    {
        const PacketComponentAssociatedData associatedData = PacketComponentAssociatedData
        {
            false,
            0.3f,
            4.f,
            EPacketHandlingType::Ack
        };
        RegisterPacketComponent<SuccessfullyConnectedToServer, NetHandler>(associatedData, &NetHandler::OnConnectionToServerSuccessful, SimpleNetLibCore::Get()->GetNetHandler());
    }

    packetComponentHandleDelegator_.SubscribeToPacketComponentDelegate
    <ServerDisconnectPacketComponent, NetHandler>(&NetHandler::OnChildDisconnectReceived, SimpleNetLibCore::Get()->GetNetHandler());

}

void PacketManager::UpdateServerPinging()
{
    using namespace std::chrono;
    
    const steady_clock::time_point currentTime = steady_clock::now();
    const duration<double> deltaTime = duration_cast<duration<double>>(currentTime - lastTimePacketSent_);
    if (deltaTime.count() > NET_TIME_UNTIL_NEXT_SERVER_PING)
    {
        const ServerPingPacketComponent serverPingPacketComponent;
        SendPacketComponent(serverPingPacketComponent, SimpleNetLibCore::Get()->GetNetHandler()->GetParentConnection());
    }
}
}
