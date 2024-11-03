#include "UserThread.h"
#include "lockers.h"
#include <list>
#include <unordered_map>

const SIZE_T defaultStackSize = 1024 * 1024;

typedef struct UserThreadInfo
{
    //线程上下文
    UserThreadContext runContext;
    //下一次执行的计数器执行时间
    LARGE_INTEGER runTime;
    UserThreadInfo() : runContext{} { runTime.QuadPart = -1; }
} UserThreadInfo, * PUserThreadInfo;

//用户线程派遣器信息
typedef struct UserThreadDispatcherInfo
{
    //派遣器派遣代码开始的寄存器上下文
    UserThreadContext context;
    class UserThreadQueue
    {
        //到达派遣器的用户线程列表
        std::list<UserThreadInfo> userThreads;
        //当前用户线程
        std::list<UserThreadInfo>::iterator currentUserThread;
        //锁
        mutable single_lock_mutex lock;
    public:
        UserThreadQueue() : userThreads{}, currentUserThread{ userThreads.end() } {}
        PUserThreadInfo push_back(const UserThreadInfo& info)
        {
            lock.lock();
            userThreads.push_back(info);
            PUserThreadInfo result = &userThreads.back();
            lock.unlock();
            return result;
        }
        bool erase_current()
        {
            lock.lock();
            bool result = false;
            if (userThreads.size() && currentUserThread != userThreads.end())
            {
                userThreads.erase(currentUserThread++);
                result = true;
            }
            lock.unlock();
            return result;
        }
        PUserThreadInfo current()
        {
            PUserThreadInfo result = NULL;
            lock.lock();
            if (userThreads.size())
            {
                if (currentUserThread == userThreads.end())
                    currentUserThread = userThreads.begin();
                result = &(*currentUserThread);
            }
            lock.unlock();
            return result;
        }
        void moveToNext()
        {
            lock.lock();
            if (userThreads.size())
            {
                if (currentUserThread == userThreads.end() ||
                    ++currentUserThread == userThreads.end())
                    currentUserThread = userThreads.begin();
            }
            lock.unlock();
        }
    } userThreadQueue;
    //辅助标志
    enum Flags
    {
        NoneFlag = 0,
        DelCurrentUserThread = 0x1,
    };
    DWORD flags;
    UserThreadDispatcherInfo() : context{}, flags{NoneFlag} {}
} UserThreadDiscpatcherInfo;

//用户线程派遣器信息得集合，使用单例模式，和线程ID关联，方便查询
class UserThreadDispatcherInfoSet
{
    std::unordered_map<DWORD, UserThreadDiscpatcherInfo*> infoSet;
    mutable read_write_lock rwlock;
    UserThreadDispatcherInfoSet() = default;
    UserThreadDispatcherInfoSet operator=(const UserThreadDispatcherInfoSet&);

    bool isExistNoLock(DWORD threadId) const
    {
        return infoSet.find(threadId) != infoSet.cend();
    }

public:
    static UserThreadDispatcherInfoSet& getInstance()
    {
        static UserThreadDispatcherInfoSet set;
        return set;
    }
    bool isExist(DWORD threadId) const
    {
        rwlock.read_lock();
        bool result = isExistNoLock(threadId);
        rwlock.read_unlock();
        return result;
    }
    bool newInfo(DWORD threadId)
    {
        rwlock.write_lock();
        bool result = !isExistNoLock(threadId) ? infoSet.emplace(threadId, new UserThreadDiscpatcherInfo()), true : false;
        rwlock.write_unlock();
        return result;
    }
    bool delInfo(DWORD threadId)
    {
        rwlock.write_lock();
        auto itor = infoSet.find(threadId);
        bool result = itor != infoSet.end() ? infoSet.erase(itor), true : false;
        rwlock.write_unlock();
        return result;
    }
    UserThreadDiscpatcherInfo* getInfo(DWORD threadId) const
    {
        rwlock.read_lock();
        auto itor = infoSet.find(threadId);
        UserThreadDiscpatcherInfo* result = itor != infoSet.cend() ? itor->second : NULL;
        rwlock.read_unlock();
        return result;
    }
};

void __fastcall UserThreadStartEntry(UserThreadEntry pEntry, void* param)
{
    pEntry(param);
    //结束线程
    EndCurrentUserThread();
}

void* CreateUserThread(UserThreadEntry pEntry, void* param)
{
    //检查用户线程调度器信息集合，是否有该内核线程的数据
    //没有则新建信息数据
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        infoSet.newInfo(currentThreadId);
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //填写用户线程信息
    UserThreadInfo info = {};
    //其他寄存器信息直接载入当前执行值
    SaveContext(&info.runContext);
    //分配栈
    info.runContext.pStack = new BYTE[defaultStackSize];
    info.runContext.StackSize = defaultStackSize;

#if defined(_M_X64)
    //填写Rsp栈信息
    info.runContext.Rsp = (QWORD)((PBYTE)info.runContext.pStack + defaultStackSize);
    //标记为Save状态，可以Restore
    info.runContext.UserThreadFlags |= CAN_RESTORE;
    //设置入口函数和参数
	info.runContext.Rip = (QWORD)UserThreadStartEntry;
	info.runContext.Rcx = (QWORD)pEntry;
	info.runContext.Rdx = (QWORD)param;
#elif defined(_M_IX86)
    //填写Esp栈信息
    info.runContext.Esp = (DWORD)((PBYTE)info.runContext.pStack + defaultStackSize);
    //标记为Save状态，可以Restore
    info.runContext.UserThreadFlags |= CAN_RESTORE;
    //设置入口函数和参数
    info.runContext.Eip = (DWORD)UserThreadStartEntry;
    info.runContext.Ecx = (DWORD)pEntry;
    info.runContext.Edx = (DWORD)param;
#endif
    //添加到用户线程列表中执行并返回该用户线程信息的地址
    return (void*)pDispatcherInfo->userThreadQueue.push_back(info);
}

BOOL EndCurrentUserThread()
{
    //检查用户线程调度器信息集合，是否有该内核线程的数据
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //标记当前线程为删除
    pDispatcherInfo->flags |= UserThreadDiscpatcherInfo::Flags::DelCurrentUserThread;
    //这里重新标记下context为save状态，方便用户线程多次返回这里
    pDispatcherInfo->context.UserThreadFlags |= CAN_RESTORE;
    //切换到用户线程调度器
    RestoreContext(&pDispatcherInfo->context);
    //不可能返回
    return TRUE;
}

BOOL PauseUserThread()
{
    //检查用户线程调度器信息集合，是否有该内核线程的数据
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    //获取该线程的调度信息
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //获取当前线程信息
    PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    if (pCurrentUserThreadInfo != NULL)
    {
        //这里重新标记下context为save状态，方便用户线程多次返回这里
        pDispatcherInfo->context.UserThreadFlags |= CAN_RESTORE;
        //保存当前寄存器数值
        SaveContext(&pCurrentUserThreadInfo->runContext);
        //切换到用户线程调度器
        RestoreContext(&pDispatcherInfo->context);
        //永远不可能返回
    }
    return TRUE;
}

BOOL PauseUserThreadWithTimeout(DWORD timeoutMs)
{
    //获取高精度计时器的频率
    LARGE_INTEGER frequency = {};
    QueryPerformanceFrequency(&frequency);

    //检查用户线程调度器信息集合，是否有该内核线程的数据
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    //获取该线程的调度信息
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //获取当前线程信息
    PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    if (pCurrentUserThreadInfo != NULL)
    {
        //计算下一次执行的计数器时间
        LARGE_INTEGER count = {};
        QueryPerformanceCounter(&count);
        pCurrentUserThreadInfo->runTime.QuadPart = count.QuadPart + frequency.QuadPart / 1000 * timeoutMs;
        //这里重新标记下context为save状态，方便用户线程多次返回这里
        pDispatcherInfo->context.UserThreadFlags |= CAN_RESTORE;
        //保存当前寄存器数值
        SaveContext(&pCurrentUserThreadInfo->runContext);
        //切换到用户线程调度器
        RestoreContext(&pDispatcherInfo->context);
        //永远不可能返回
    }
    return TRUE;
}

BOOL UserThreadDispatcher()
{
    //检查用户线程调度器信息集合，是否有该内核线程的数据
    //没有则新建信息数据
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    SaveContext(&pDispatcherInfo->context);
    //如果有删除当前线程的标记，删除当前线程
    if (pDispatcherInfo->flags & UserThreadDiscpatcherInfo::Flags::DelCurrentUserThread)
    {
        PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
        if (pCurrentUserThreadInfo != NULL)
        {
            //删除用户线程栈
            if (pCurrentUserThreadInfo->runContext.pStack != NULL)
                delete[] pCurrentUserThreadInfo->runContext.pStack;
            //从用户线程列表中删除该用户线程
            pDispatcherInfo->userThreadQueue.erase_current();
        }
        pDispatcherInfo->flags &= ~UserThreadDiscpatcherInfo::Flags::DelCurrentUserThread;
    }
    //按照链表顺序执行用户线程
    pDispatcherInfo->userThreadQueue.moveToNext();
    PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    //这里设置为while循环，目的是找第一个到达执行时间的用户线程
    while (pCurrentUserThreadInfo != NULL)
    {
        //可以立即执行的话跳出循环
        if (pCurrentUserThreadInfo->runTime.QuadPart == -1)
            break;
        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);
        //到达执行时间的话跳出循环
        if (pCurrentUserThreadInfo->runTime.QuadPart <= count.QuadPart)
        {
            pCurrentUserThreadInfo->runTime.QuadPart = -1;
            break;
        }
        //如果没有到达执行时间，转到下一个
        pDispatcherInfo->userThreadQueue.moveToNext();
        pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    }
    //可能是NULL，需要NULL检查
    if (pCurrentUserThreadInfo != NULL)
        RestoreContext(&pCurrentUserThreadInfo->runContext);
    infoSet.delInfo(currentThreadId);
    return TRUE;
}
