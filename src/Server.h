#pragma once
#include <vector>
#include "Constants/Constants.h"
#include "Threadheader.h"

class AcceptThread;

using namespace std;

class Server
{
private:
    // Winsock 시작
    WSAData wsa;

    // IOCP 핸들
    HANDLE IOCPHandle;

    /*리슨 소켓*/
    SOCKET listen_sock;

    /*서버 주소 구조체*/
    sockaddr_in server_addr;

    AcceptThread *accept_thread;

    IOCPThread *iocp_thread;

    /*클라이언트 배열*/
    vector<ClientInfo *> client_arr;

    /*클라이언트 배열 Lock*/
    CRITICAL_SECTION client_arr_cs;

public:
    inline static Server *Instance = nullptr;

    Server();

    ~Server();

    /*초기화*/
    int Initialize();

    /*클라이언트 접속 시 호출*/
    void ClientPush(ClientInfo *client);

    HANDLE *GetIOCPHandle();
};