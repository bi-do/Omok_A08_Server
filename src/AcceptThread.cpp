#include "AcceptThread.h"

AcceptThread::AcceptThread(SOCKET &listen_sock) : listen_sock(listen_sock)
{
    this->addr_size = sizeof(sockaddr_in);
}

AcceptThread::~AcceptThread()
{
}

void AcceptThread::Run()
{
    HANDLE accept_handle_temp = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread::AcceptLoop, this, 0, nullptr);
    CloseHandle(accept_handle_temp);
}

unsigned int AcceptThread::AcceptLoop(void *param)
{
    AcceptThread *self = (AcceptThread *)param;
    SOCKET client_sock;
    WSABUF wsabuf{}; // Recv 버퍼 주소 전달용 변수
    DWORD flags = 0; // Recv 플래그 용 변수

    while (1)
    {
        client_sock = accept(self->listen_sock, (sockaddr *)&self->client_addr, &self->addr_size);
        if (client_sock == INVALID_SOCKET)
        {
            cout << "클라이언트 소켓 Accept 실패. 실패 코드 : " << GetLastError() << endl;
            break;
        }
        else
        {
            WSAOVERLAPPED *over = new WSAOVERLAPPED{};
            ClientInfo *client = new ClientInfo{client_sock, self->client_addr, *over};

            Server::Instance->ClientPush(client);

            // IOCP 큐에 감시 대상 추가
            CreateIoCompletionPort((HANDLE)client_sock, Server::Instance->GetIOCPHandle(), (ULONG_PTR)client, 0);

            // IOCP 큐에 소켓 걸어주기
            WSARecv(client_sock, &wsabuf, 1, nullptr, &flags, over, nullptr);
        }
    }

    return 0;
}