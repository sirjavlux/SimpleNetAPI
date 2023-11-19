﻿#include "../SimpleNetAPI/Packet/Packet.h"
#include "../SimpleNetAPI/Packet/PacketComponent.h"
#include "../SimpleNetAPI/Packet/PacketComponentHandleDelegator.h"
#include "../SimpleNetAPI/Packet/PacketManager.h"
#include "gtest/gtest.h"

class TestComponent : public PacketComponent
{
public:
    TestComponent() : PacketComponent(0, sizeof(TestComponent))
    { }
    
    int integerValue = 0;
};

class InvalidComponent : public PacketComponent
{
public:
    InvalidComponent() : PacketComponent(0, 0)
    { }
};

TEST(PacketTests, PacketCreation)
{
    Packet packet = Packet(
        EPacketPacketType::ClientToServer, EPacketHandlingType::None);

    for (int i = 0; i < 20; ++i)
    {
        TestComponent testComponent;
        testComponent.integerValue = i;
        packet.AddComponent(testComponent);
    }

    std::vector<PacketComponent*> outComponents;
    packet.GetComponents(outComponents);

    int iter = 0;
    for (const PacketComponent* component : outComponents)
    {
        const TestComponent* castedComponent = reinterpret_cast<const TestComponent*>(component);
        
        EXPECT_TRUE(castedComponent != nullptr);
        EXPECT_EQ(iter, castedComponent->integerValue);

        ++iter;
    }
}

void TestComponentFunction(const TestComponent& InPacketComponent)
{
    EXPECT_EQ(InPacketComponent.integerValue, 20);
    std::cout << InPacketComponent.integerValue << std::endl;
}

void InvalidComponentFunction(const InvalidComponent& InPacketComponent)
{
    
}

TEST(PacketTests, PacketManagerComponentRegestring)
{
    PacketManager* packetManager = PacketManager::Initialize(EPacketManagerType::Client);
    
    TestComponent validComponent;
    InvalidComponent invalidComponent;

    bool failed = false;
    try
    {
        packetManager->RegisterPacketComponent<InvalidComponent>(EPacketHandlingType::None, &InvalidComponentFunction);
    }
    catch (...)
    {
        failed = true;
    }

    EXPECT_TRUE(failed);

    failed = false;
    try
    {
        packetManager->RegisterPacketComponent<TestComponent>(EPacketHandlingType::None, &TestComponentFunction);
    }
    catch (...)
    {
        failed = true;
    }

    EXPECT_FALSE(failed);
}

TEST(PacketDelegateTests, HandleDelegate)
{
    PacketComponentHandleDelegator delegator;
    TestComponent component;
    component.integerValue = 20;
    
    delegator.MapComponentHandleDelegate<TestComponent>(&TestComponentFunction);

    delegator.HandleComponent(component);
}

class DynamicDelegateHandleClass
{
public:
    int testInt = 0;
    
    void TestComponentFunction(const TestComponent& InPacketComponent)
    {
        EXPECT_EQ(InPacketComponent.integerValue, 20);
        EXPECT_EQ(testInt, 30);
        std::cout << InPacketComponent.integerValue << std::endl;
        std::cout << testInt << std::endl;
    }
};

TEST(PacketDelegateTests, DynamicHandleDelegate)
{
    PacketComponentHandleDelegator delegator;

    DynamicDelegateHandleClass delegateOwner;
    delegateOwner.testInt = 30;
    
    TestComponent component;
    component.integerValue = 20;
    
    delegator.MapComponentHandleDelegateDynamic<TestComponent, DynamicDelegateHandleClass>(&DynamicDelegateHandleClass::TestComponentFunction, &delegateOwner);

    delegator.HandleComponent(component);
}

TEST(PacketTests, SendPacket)
{
    PacketManager* packetManager = PacketManager::Initialize(EPacketManagerType::Client);

    packetManager->RegisterPacketComponent<TestComponent>(EPacketHandlingType::None, &TestComponentFunction);
    
    for (int i = 0; i < 100000; ++i)
    {
        TestComponent testComponent;
        testComponent.integerValue = i;
        
        EXPECT_TRUE(packetManager->SendPacketComponent<TestComponent>(testComponent));
    }
}