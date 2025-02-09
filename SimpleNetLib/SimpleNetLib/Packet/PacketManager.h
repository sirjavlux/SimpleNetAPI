﻿#pragma once

#include "PacketComponent.h"
#include "PacketTargetData.h"
#include "PacketComponentRegistry.h"
#include "../Network/NetHandler.h"
#include "../Utility/HashUtility.hpp"

class ReturnAckComponent;

class NetHandler;
enum class EPacketHandlingType : uint8_t;

enum class ENetworkHandleType
{
    None    = 0,
    Server  = 1,
    Client  = 2,
};

namespace Net
{
class PacketManager
{
public:
    PacketManager();
    ~PacketManager();
    
    static PacketManager* Initialize();
    static PacketManager* Get();
    static void End();

    template<typename ComponentType>
    bool SendPacketComponentToParent(const ComponentType& InPacketComponent);
    
    template<typename ComponentType>
    bool SendPacketComponent(const ComponentType& InPacketComponent, const sockaddr_storage& InTarget, const PacketComponentAssociatedData* InCustomAssociatedData = nullptr);

    template<typename ComponentType>
    bool SendPacketComponentWithLod(const ComponentType& InPacketComponent, const NetUtility::NetVector3& InPosition, const sockaddr_storage& InTarget, const PacketComponentAssociatedData* InCustomAssociatedData = nullptr);

    // Keep in mind, this can be a bit expensive due to no lod or culling
    template<typename ComponentType>
    bool SendPacketComponentMulticast(const ComponentType& InPacketComponent);
    
    template<typename ComponentType>
    bool SendPacketComponentMulticastWithLod(const ComponentType& InPacketComponent, const NetUtility::NetVector3& InPosition, const PacketComponentAssociatedData* InCustomAssociatedData = nullptr);

    ENetworkHandleType GetManagerType() const { return managerType_; }
    
    void UpdateClientNetPosition(const sockaddr_storage& InAddress, const NetUtility::NetVector3& InPosition);

    float GetDeltaTime() const { return lastDeltaTime_; }

    bool IsServer() const { return managerType_ == ENetworkHandleType::Server; }

    int GetUpdateIterator() const { return updateIterator_; }
    
private:
    void Update();
    
    void HandleComponent(const sockaddr_storage& InComponentSender, const PacketComponent& InPacketComponent);
    
    void FixedUpdate();
    void UpdatePacketsWaitingForReturnAck(const sockaddr_storage& InTarget, PacketTargetData& InTargetData) const;
    void UpdatePacketsToSend(const sockaddr_storage& InTarget, PacketTargetData& InTargetData) const;
    
    std::chrono::steady_clock::time_point lastUpdateTime_;
    double updateLag_ = 0.0;

    std::chrono::steady_clock::time_point lastUpdateFinishedTime_;
    
    bool DoesUpdateIterMatchPacketFrequency(const PacketFrequencyData& InPacketFrequencyData) const;
    
    static void OnNetTargetConnected(const sockaddr_storage& InTarget);
    static void OnNetTargetDisconnection(const sockaddr_storage& InTarget, uint8_t InDisconnectType);

    void OnAckReturnReceived(const sockaddr_storage& InTarget, const PacketComponent& InComponent);

    void SubscribeToPacketComponents();
    void UnSubscribeFromDelegates();
    
    void UpdateServerPinging();
    std::chrono::steady_clock::time_point lastTimePacketSent_;
    
    ENetworkHandleType managerType_;
    
    static PacketManager* instance_;

    std::unordered_map<sockaddr_storage, PacketTargetData> packetTargetDataMap_;

    int updateIterator_ = 0;

    float lastDeltaTime_ = 0.f;
    
    friend class NetHandler;
    friend class SimpleNetLibCore;
};

template <typename ComponentType>
bool PacketManager::SendPacketComponentToParent(const ComponentType& InPacketComponent)
{
    if (SimpleNetLibCore::Get()->GetNetHandler()->IsServer())
    {
        return false;
    }

    return SendPacketComponent<ComponentType>(InPacketComponent, SimpleNetLibCore::Get()->GetNetHandler()->GetParentConnection());
}

template <typename ComponentType>
bool PacketManager::SendPacketComponent(const ComponentType& InPacketComponent, const sockaddr_storage& InTarget, const PacketComponentAssociatedData* InCustomAssociatedData)
{
    lastTimePacketSent_ = std::chrono::steady_clock::now(); // Reset server ping timer
    
    // Check Component Validity
    if (!SimpleNetLibCore::Get()->GetPacketComponentRegistry()->IsPacketComponentValid<ComponentType>() && InPacketComponent.GetSize() <= NET_PACKET_COMPONENT_DATA_SIZE_TOTAL)
    {
        throw std::runtime_error("ComponentType isn't a valid PacketComponent. Make sure size is set and inside bounds.");
    }
    
    PacketTargetData& packetTargetData = packetTargetDataMap_.at(InTarget);

    packetTargetData.AddPacketComponentToSend(std::make_shared<ComponentType>(InPacketComponent), InCustomAssociatedData);
    
    return true;
}

template <typename ComponentType>
bool PacketManager::SendPacketComponentWithLod(const ComponentType& InPacketComponent, const NetUtility::NetVector3& InPosition, const sockaddr_storage& InTarget, const PacketComponentAssociatedData* InCustomAssociatedData)
{
    const std::unordered_map<sockaddr_in, NetTarget>& connections = SimpleNetLibCore::Get()->GetNetHandler()->GetNetConnectionHandler().GetConnections();
    const sockaddr_in ipv4Address = NetUtility::RetrieveIPv4AddressFromStorage(InTarget);

    if (connections.find(ipv4Address) == connections.end())
    {
        return false;
    }
    
    const NetTarget& connection = connections.at(ipv4Address);
    
    // Check associated data validity
    const PacketComponentAssociatedData* associatedData = SimpleNetLibCore::Get()->GetPacketComponentRegistry()->FetchPacketComponentAssociatedData(InPacketComponent.GetIdentifier());
    if (!associatedData)
    {
        return false;
    }
            
    // Preliminary culling check
    const float distanceSqr = InPosition.DistanceSqr(connection.netCullingPosition);
    if (associatedData->distanceToCullPacketComponentAt > -1
        && distanceSqr > std::powf(associatedData->distanceToCullPacketComponentAt, 2.f))
    {
        return false; // Continue if culling distance is exceeded
    }
    
    PacketTargetData& packetTargetData = packetTargetDataMap_.at(InTarget);
            
    // Send packet with the correct lod frequency
    packetTargetData.AddPacketComponentToSendWithLod(std::make_shared<ComponentType>(InPacketComponent), distanceSqr, InCustomAssociatedData);

    return true;
}

template <typename ComponentType>
bool PacketManager::SendPacketComponentMulticast(const ComponentType& InPacketComponent)
{
    // Can't multicast upstream from client to server
    if (SimpleNetLibCore::Get()->GetNetHandler()->IsServer())
    {
        const std::unordered_map<sockaddr_in, NetTarget>& connections = SimpleNetLibCore::Get()->GetNetHandler()->GetNetConnectionHandler().GetConnections();
        for (const auto& connection : connections)
        {
            SendPacketComponent<ComponentType>(InPacketComponent, NetUtility::RetrieveStorageFromIPv4Address(connection.first));
        }
        return true;
    }
    return false;
}

template <typename ComponentType>
bool PacketManager::SendPacketComponentMulticastWithLod(const ComponentType& InPacketComponent, const NetUtility::NetVector3& InPosition, const PacketComponentAssociatedData* InCustomAssociatedData)
{
    // Can't multicast upstream from client to server
    if (SimpleNetLibCore::Get()->GetNetHandler()->IsServer())
    {
        const std::unordered_map<sockaddr_in, NetTarget>& connections = SimpleNetLibCore::Get()->GetNetHandler()->GetNetConnectionHandler().GetConnections();
        for (const auto& connection : connections)
        {
            // Check associated data validity
            const PacketComponentAssociatedData* associatedData = SimpleNetLibCore::Get()->GetPacketComponentRegistry()->FetchPacketComponentAssociatedData(InPacketComponent.GetIdentifier());
            if (!associatedData)
            {
                continue;
            }
            
            // Preliminary culling check
            const float distanceSqr = InPosition.DistanceSqr(connection.second.netCullingPosition);
            if (associatedData->distanceToCullPacketComponentAt > -1
                && distanceSqr > std::powf(associatedData->distanceToCullPacketComponentAt, 2.f))
            {
                continue; // Continue if culling distance is exceeded
            }

            const sockaddr_storage storageAddress = NetUtility::RetrieveStorageFromIPv4Address(connection.first);
            PacketTargetData& packetTargetData = packetTargetDataMap_.at(storageAddress);
            
            // Send packet with the correct lod frequency
            packetTargetData.AddPacketComponentToSendWithLod(std::make_shared<ComponentType>(InPacketComponent), distanceSqr, InCustomAssociatedData);
        }
        return true;
    }
    return false;
}
}