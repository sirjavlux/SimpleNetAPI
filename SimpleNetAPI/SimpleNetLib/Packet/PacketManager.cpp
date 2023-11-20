﻿#include "PacketManager.h"

PacketManager* PacketManager::instance_ = nullptr;

PacketManager::PacketManager(const EPacketManagerType InPacketManagerType, const NetSettings& InNetSettings)
    : managerType_(InPacketManagerType), netHandler_(InNetSettings)
{
    
}

PacketManager* PacketManager::Initialize(const EPacketManagerType InPacketManagerType, const NetSettings& InNetSettings)
{
    if (instance_ == nullptr)
    {
        instance_ = new PacketManager(InPacketManagerType, InNetSettings);
    }
    
    return instance_;
}

PacketManager* PacketManager::Get()
{
    return instance_;
}

void PacketManager::Update()
{
    using namespace std::chrono;

    const steady_clock::time_point currentTime = steady_clock::now();
    const duration<double> deltaTime = duration_cast<duration<double>>(currentTime - lastUpdateTime_);
    lastUpdateTime_ = currentTime;

    updateLag_ += deltaTime.count();

    if (updateLag_ >= FIXED_UPDATE_TIME)
    {
        FixedUpdate();
        updateLag_ -= FIXED_UPDATE_TIME;
    }
}

void PacketManager::FixedUpdate()
{
    
}
