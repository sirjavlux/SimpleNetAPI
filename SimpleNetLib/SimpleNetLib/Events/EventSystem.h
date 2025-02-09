﻿#pragma once

#include "../NetIncludes.h"

namespace Net
{
    class EventSystem
    {
    public:
        static EventSystem* Initialize();
        static EventSystem* Get();
        static void End();

        EventSystem();
        ~EventSystem();

        // On disconnect locally/closing server locally
        DynamicMulticastDelegate<> onCloseConnectionEvent;
        
        // Connection events
        DynamicMulticastDelegate<const sockaddr_storage&> onClientConnectEvent;
        DynamicMulticastDelegate<const sockaddr_storage&> onConnectedToServerEvent;
        DynamicMulticastDelegate<const sockaddr_storage&, uint8_t> onClientDisconnectEvent;
        
    private:
        static EventSystem* instance_;
    };
}