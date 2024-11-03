#include "Context.h"
#include "UserThread.h"
#include "lockers.h"
#include <stdio.h>
#include <process.h>
#include <Windows.h>

void func(void* param)
{
    PauseUserThreadWithTimeout(1000);
    if((SIZE_T)param < 10)
        CreateUserThread(func, ((char*)param) + 1);
    printf("param = %p, aaaa\n", param);
    return;
}

struct TestInfo
{
    unsigned int _cnt;
    read_write_lock rwlock;
};

unsigned __stdcall read_func(void* param)
{
    TestInfo* pInfo = (TestInfo*)param;
    unsigned int _cnt_cpy = 0;
    do
    {
        pInfo->rwlock.read_lock();
        printf("value: %d\n", pInfo->_cnt);
        _cnt_cpy = pInfo->_cnt;
        pInfo->rwlock.read_unlock();
        Sleep(20);
    } while (_cnt_cpy < 100);
    return 0;
    
}
unsigned __stdcall write_func(void* param)
{
    TestInfo* pInfo = (TestInfo*)param;
    for (unsigned i = 0; i < 50; ++i)
    {
        pInfo->rwlock.write_lock();
        ++pInfo->_cnt;
        pInfo->rwlock.write_unlock();
        Sleep(20);
    }
    return 0;
}

int main()
{
    /*TestInfo info = {};

    HANDLE hThread1 = (HANDLE)_beginthreadex(NULL, 0, read_func, &info, 0, NULL);
    HANDLE hThread2 = (HANDLE)_beginthreadex(NULL, 0, read_func, &info, 0, NULL);
    HANDLE hThread3 = (HANDLE)_beginthreadex(NULL, 0, write_func, &info, 0, NULL);
    HANDLE hThread4 = (HANDLE)_beginthreadex(NULL, 0, write_func, &info, 0, NULL);

    WaitForSingleObject(hThread1, INFINITE);
    WaitForSingleObject(hThread2, INFINITE);
    WaitForSingleObject(hThread3, INFINITE);
    WaitForSingleObject(hThread4, INFINITE);*/

    CreateUserThread(func, (void*)0);
    UserThreadDispatcher();
}

