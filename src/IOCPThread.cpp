#include "IOCPThread.h"

void IOCPThread::Run(int thread_cnt)
{
    for (int i = 0; i < thread_cnt; i++)
    {
        HANDLE IOCP_handle_temp = (HANDLE)_beginthreadex(nullptr, 0, IOCPThread::IOCPStart, this, 0, nullptr);
        CloseHandle(IOCP_handle_temp);
    }
}

unsigned int IOCPThread::IOCPStart(void *param)
{
    IOCPThread *self = (IOCPThread *)param;

    return 1;
}