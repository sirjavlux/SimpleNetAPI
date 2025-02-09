﻿#include "PacketComponentDelegator.h"

#include "../Packet/PacketComponent.h"

namespace Net
{
void PacketComponentDelegator::HandleComponent(const sockaddr_storage& InNetTarget, const PacketComponent& InPacketComponent)
{
    const uint16_t identifier = InPacketComponent.GetIdentifier();

    // Dynamic Delegate
    const auto iter = delegatesDynamic_.find(identifier);
    if (iter != delegatesDynamic_.end())
    {
        iter->second.Execute(InNetTarget, InPacketComponent);
    }

    // Static Delegate
    const auto iterStatic = delegatesStatic_.find(identifier);
    if (iterStatic != delegatesStatic_.end())
    {
        iterStatic->second.Execute(InNetTarget, InPacketComponent);
    }
}
}