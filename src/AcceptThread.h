#pragma once
#include "Constants/header.h"
#include "Constants/Constants.h"
#include "Server.h"

using namespace std;

class Server;

class AcceptThread
{
private:
    /*리슨 소켓*/
    SOCKET &listen_sock;

    /*클라이언트 주소용 버퍼*/
    sockaddr_in client_addr = {};

    /*클라이언트 주소 버퍼 크기*/
    int addr_size;

public:
    AcceptThread(SOCKET &listen_sock);

    ~AcceptThread();

    /*Accept 스레드 시작*/
    void Run();

    static unsigned int AcceptLoop(void *param);
};
