﻿#pragma once

#include "PacketComponentTypes.hpp"
#include "Packet/PacketComponent.h"

class RequestSpawnEntityComponent : public Net::PacketComponent
{
public:
  RequestSpawnEntityComponent();
};

inline RequestSpawnEntityComponent::RequestSpawnEntityComponent()
    : PacketComponent(static_cast<int16_t>(EPacketComponentType::RequestSpawnEntity), sizeof(RequestSpawnEntityComponent))
{ }