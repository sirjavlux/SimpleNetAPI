#pragma once

#include "../NetIncludes.h"

class PacketComponent;

enum class NET_LIB_EXPORT EPacketHandlingType : uint8_t
{
    None		= 0,
    Ack			= 1,
    
    Size        = 3
};

/*
enum class NET_LIB_EXPORT EPacketPacketType : uint8_t
{
    None			= 0,
    ClientToServer	= 1,
    ServerToClient	= 2,
};
*/

enum class NET_LIB_EXPORT EAddComponentResult : uint8_t
{
    Success			    = 0,
    InvalidComponent	= 1,
    SizeOutOfBounds	    = 2,
};

struct NET_LIB_EXPORT PacketComponentData
{
    uint16_t size = 0;
    const PacketComponent* packetComponent;
};

class NET_LIB_EXPORT Packet
{
public:
    Packet(EPacketHandlingType InPacketHandlingType);
    Packet(const char* InBuffer, const int InBytesReceived);
    
    bool IsValid() const;
    bool IsEmpty() const;
    
    EAddComponentResult AddComponent(const PacketComponent& InPacketComponent);
    
    void GetComponents(std::vector<const PacketComponent*>& OutComponents) const;

    void Reset();

    const char* GetData() const { return &data_[0]; }

    int32_t GetIdentifier() const { return packetIdentifier_; }

    EPacketHandlingType GetPacketType() const { return packetHandlingType_; }
    
private:
    void ExtractComponent(std::vector<const PacketComponent*>& OutComponents, int& Iterator) const;

    int32_t packetIdentifier_ = INT32_MIN;

    EPacketHandlingType packetHandlingType_ = EPacketHandlingType::None;
    
    static int32_t GenerateIdentifier();
    
    char data_[NET_PACKET_COMPONENT_DATA_SIZE_TOTAL];

    // Data bypassing the packet size limit
    // THIS DATA IS NOT INCLUDED IN PACKET NET TRANSFERS
    uint16_t packetDataIter_ = 0;
};
