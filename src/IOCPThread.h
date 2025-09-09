#pragma once
#include <windows.h>
#include <iostream>

class IOCPThread
{
public:
    void Run(int thread_cnt);

    static unsigned int IOCPStart(void *param);
};