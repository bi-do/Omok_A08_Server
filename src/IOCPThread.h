#pragma once
#include "Constants/header.h"
#include "Server.h"

class IOCPThread
{
    DWORD recv_size = 0;

    ClientInfo * client;

    LPOVERLAPPED client_over;

    DWORD recv_flags = 0;

public:
    void Run();

    static unsigned int IOCPStart(void *param);

};