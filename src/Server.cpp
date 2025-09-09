#include "Server.h"

Server::Server()
{
    if (Server::Instance == nullptr)
    {
        Server::Instance = this;
    }
    else
    {
        delete this;
    }
}

Server::~Server()
{
    delete this->accept_thread;
    delete this->iocp_thread;

    closesocket(this->listen_sock);
    CloseHandle(this->IOCPHandle);

    EnterCriticalSection(&this->client_arr_cs);
    for (ClientInfo *element : this->client_arr)
    {
        delete element;
    }
    LeaveCriticalSection(&this->client_arr_cs);

    DeleteCriticalSection(&this->client_arr_cs);
    WSACleanup();

    cout << "서버 종료 완료" << endl;
}

int Server::Initialize()
{
    // 임계 구간 초기화
    InitializeCriticalSection(&this->client_arr_cs);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    // IOCP 모듈 생성
    this->IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    // IOCP 소켓 생성
    this->listen_sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (this->listen_sock == INVALID_SOCKET)
    {
        cout << "리슨 소켓 생성 실패. 실패코드 : " << GetLastError() << endl;
        return SOCKET_ERR;
    }

    // 서버 주소 설정
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_port = htons(ACCEPT_PORT);
    inet_pton(AF_INET, ACCEPT_IP, &this->server_addr.sin_addr);

    // 소켓 바인딩
    if (SOCKET_ERROR == bind(this->listen_sock, (sockaddr *)&this->server_addr, sizeof(sockaddr_in)))
    {
        cout << "소켓 바인딩 에러. 코드 : " << GetLastError() << endl;
        return BIND_ERR;
    }

    // IOCP 스레드 생성
    this->iocp_thread = new IOCPThread{};
    this->iocp_thread->Run(IOCPTHREAD_CNT);

    // 소켓 리슨
    if (SOCKET_ERROR == listen(this->listen_sock, SOMAXCONN))
    {
        cout << "소켓 리슨 에러. 코드 : " << GetLastError() << endl;
        return BIND_ERR;
    }

    // Accept 스레드 시작
    this->accept_thread = new AcceptThread{this->listen_sock};
    this->accept_thread->Run();

    cout << "서버 구동 시작" << endl;

    return 5;
}

void Server::ClientPush(ClientInfo *client)
{
    EnterCriticalSection(&this->client_arr_cs);
    this->client_arr.push_back(client);
    LeaveCriticalSection(&this->client_arr_cs);
    char IPtemp[20]{};

    inet_ntop(AF_INET, &client->client_addr.sin_addr, IPtemp, sizeof(IPtemp));

    cout << "클라이언트 접속. IP : " << IPtemp << endl;
}

HANDLE *Server::GetIOCPHandle()
{
    return &this->IOCPHandle;
}