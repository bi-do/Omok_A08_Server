#include "IOCPThread.h"

void IOCPThread::Run()
{
    HANDLE IOCP_handle_temp = (HANDLE)_beginthreadex(nullptr, 0, IOCPThread::IOCPStart, this, 0, nullptr);
    CloseHandle(IOCP_handle_temp);
}

unsigned int IOCPThread::IOCPStart(void *param)
{
    IOCPThread *self = (IOCPThread *)param;

    HANDLE IOCP = Server::Instance->GetIOCPHandle();

    bool result;

    while (1)
    {
        result = GetQueuedCompletionStatus(IOCP, &self->recv_size, (PULONG_PTR)&self->client, &self->client_over, INFINITE);
        if (!result) // GQCS 실패
        {
            if (self->client_over == nullptr) // IOCP 핸들 CLOSE
            {
                // cout << GetLastError() << endl;
                break;
            }
            else
            {
                cout << "GQCS Fail . ERR Code : " << GetLastError() << endl;
                return 1;
            }
        }
        else if (self->recv_size == 0) // 클라이언트가 연결을 종료했을 시
        {
            Server::Instance->ClientRemove(self->client);
        }
        else
        {
            Buffer *copy_buf = new Buffer{};
            memcpy(copy_buf, &self->client->buf, sizeof(Header)+self->client->buf.header.body_len);

            Server::Instance->ParseAndWork(self->client, copy_buf);
            WSABUF wsabuf{};
            wsabuf.buf = (char *)&self->client->buf;
            wsabuf.len = sizeof(Buffer);

            WSARecv(self->client->client_sock, &wsabuf,1,nullptr,&self->recv_flags,self->client->over,nullptr);
        }
    }
    // cout << "GQCS Thread End" << endl;

    return 0;
}