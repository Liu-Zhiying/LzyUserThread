#include "UserThread.h"
#include "lockers.h"
#include <list>
#include <unordered_map>

const SIZE_T defaultStackSize = 1024 * 1024;

typedef struct UserThreadInfo
{
    //�߳�������
    UserThreadContext runContext;
    //��һ��ִ�еļ�����ִ��ʱ��
    LARGE_INTEGER runTime;
    UserThreadInfo() : runContext{} { runTime.QuadPart = -1; }
} UserThreadInfo, * PUserThreadInfo;

//�û��߳���ǲ����Ϣ
typedef struct UserThreadDispatcherInfo
{
    //��ǲ����ǲ���뿪ʼ�ļĴ���������
    UserThreadContext context;
    class UserThreadQueue
    {
        //������ǲ�����û��߳��б�
        std::list<UserThreadInfo> userThreads;
        //��ǰ�û��߳�
        std::list<UserThreadInfo>::iterator currentUserThread;
        //��
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
    //������־
    enum Flags
    {
        NoneFlag = 0,
        DelCurrentUserThread = 0x1,
    };
    DWORD flags;
    UserThreadDispatcherInfo() : context{}, flags{NoneFlag} {}
} UserThreadDiscpatcherInfo;

//�û��߳���ǲ����Ϣ�ü��ϣ�ʹ�õ���ģʽ�����߳�ID�����������ѯ
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
    //�����߳�
    EndCurrentUserThread();
}

void* CreateUserThread(UserThreadEntry pEntry, void* param)
{
    //����û��̵߳�������Ϣ���ϣ��Ƿ��и��ں��̵߳�����
    //û�����½���Ϣ����
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        infoSet.newInfo(currentThreadId);
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //��д�û��߳���Ϣ
    UserThreadInfo info = {};
    //�����Ĵ�����Ϣֱ�����뵱ǰִ��ֵ
    SaveContext(&info.runContext);
    //����ջ
    info.runContext.pStack = new BYTE[defaultStackSize];
    info.runContext.StackSize = defaultStackSize;

#if defined(_M_X64)
    //��дRspջ��Ϣ
    info.runContext.Rsp = (QWORD)((PBYTE)info.runContext.pStack + defaultStackSize);
    //���ΪSave״̬������Restore
    info.runContext.UserThreadFlags |= CAN_RESTORE;
    //������ں����Ͳ���
	info.runContext.Rip = (QWORD)UserThreadStartEntry;
	info.runContext.Rcx = (QWORD)pEntry;
	info.runContext.Rdx = (QWORD)param;
#elif defined(_M_IX86)
    //��дEspջ��Ϣ
    info.runContext.Esp = (DWORD)((PBYTE)info.runContext.pStack + defaultStackSize);
    //���ΪSave״̬������Restore
    info.runContext.UserThreadFlags |= CAN_RESTORE;
    //������ں����Ͳ���
    info.runContext.Eip = (DWORD)UserThreadStartEntry;
    info.runContext.Ecx = (DWORD)pEntry;
    info.runContext.Edx = (DWORD)param;
#endif
    //��ӵ��û��߳��б���ִ�в����ظ��û��߳���Ϣ�ĵ�ַ
    return (void*)pDispatcherInfo->userThreadQueue.push_back(info);
}

BOOL EndCurrentUserThread()
{
    //����û��̵߳�������Ϣ���ϣ��Ƿ��и��ں��̵߳�����
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //��ǵ�ǰ�߳�Ϊɾ��
    pDispatcherInfo->flags |= UserThreadDiscpatcherInfo::Flags::DelCurrentUserThread;
    //�������±����contextΪsave״̬�������û��̶߳�η�������
    pDispatcherInfo->context.UserThreadFlags |= CAN_RESTORE;
    //�л����û��̵߳�����
    RestoreContext(&pDispatcherInfo->context);
    //�����ܷ���
    return TRUE;
}

BOOL PauseUserThread()
{
    //����û��̵߳�������Ϣ���ϣ��Ƿ��и��ں��̵߳�����
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    //��ȡ���̵߳ĵ�����Ϣ
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //��ȡ��ǰ�߳���Ϣ
    PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    if (pCurrentUserThreadInfo != NULL)
    {
        //�������±����contextΪsave״̬�������û��̶߳�η�������
        pDispatcherInfo->context.UserThreadFlags |= CAN_RESTORE;
        //���浱ǰ�Ĵ�����ֵ
        SaveContext(&pCurrentUserThreadInfo->runContext);
        //�л����û��̵߳�����
        RestoreContext(&pDispatcherInfo->context);
        //��Զ�����ܷ���
    }
    return TRUE;
}

BOOL PauseUserThreadWithTimeout(DWORD timeoutMs)
{
    //��ȡ�߾��ȼ�ʱ����Ƶ��
    LARGE_INTEGER frequency = {};
    QueryPerformanceFrequency(&frequency);

    //����û��̵߳�������Ϣ���ϣ��Ƿ��и��ں��̵߳�����
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    //��ȡ���̵߳ĵ�����Ϣ
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    //��ȡ��ǰ�߳���Ϣ
    PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    if (pCurrentUserThreadInfo != NULL)
    {
        //������һ��ִ�еļ�����ʱ��
        LARGE_INTEGER count = {};
        QueryPerformanceCounter(&count);
        pCurrentUserThreadInfo->runTime.QuadPart = count.QuadPart + frequency.QuadPart / 1000 * timeoutMs;
        //�������±����contextΪsave״̬�������û��̶߳�η�������
        pDispatcherInfo->context.UserThreadFlags |= CAN_RESTORE;
        //���浱ǰ�Ĵ�����ֵ
        SaveContext(&pCurrentUserThreadInfo->runContext);
        //�л����û��̵߳�����
        RestoreContext(&pDispatcherInfo->context);
        //��Զ�����ܷ���
    }
    return TRUE;
}

BOOL UserThreadDispatcher()
{
    //����û��̵߳�������Ϣ���ϣ��Ƿ��и��ں��̵߳�����
    //û�����½���Ϣ����
    UserThreadDispatcherInfoSet& infoSet = UserThreadDispatcherInfoSet::getInstance();
    DWORD currentThreadId = GetThreadId(GetCurrentThread());
    if (!infoSet.isExist(currentThreadId))
        return FALSE;
    UserThreadDiscpatcherInfo* pDispatcherInfo = infoSet.getInfo(currentThreadId);
    SaveContext(&pDispatcherInfo->context);
    //�����ɾ����ǰ�̵߳ı�ǣ�ɾ����ǰ�߳�
    if (pDispatcherInfo->flags & UserThreadDiscpatcherInfo::Flags::DelCurrentUserThread)
    {
        PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
        if (pCurrentUserThreadInfo != NULL)
        {
            //ɾ���û��߳�ջ
            if (pCurrentUserThreadInfo->runContext.pStack != NULL)
                delete[] pCurrentUserThreadInfo->runContext.pStack;
            //���û��߳��б���ɾ�����û��߳�
            pDispatcherInfo->userThreadQueue.erase_current();
        }
        pDispatcherInfo->flags &= ~UserThreadDiscpatcherInfo::Flags::DelCurrentUserThread;
    }
    //��������˳��ִ���û��߳�
    pDispatcherInfo->userThreadQueue.moveToNext();
    PUserThreadInfo pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    //��������Ϊwhileѭ����Ŀ�����ҵ�һ������ִ��ʱ����û��߳�
    while (pCurrentUserThreadInfo != NULL)
    {
        //��������ִ�еĻ�����ѭ��
        if (pCurrentUserThreadInfo->runTime.QuadPart == -1)
            break;
        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);
        //����ִ��ʱ��Ļ�����ѭ��
        if (pCurrentUserThreadInfo->runTime.QuadPart <= count.QuadPart)
        {
            pCurrentUserThreadInfo->runTime.QuadPart = -1;
            break;
        }
        //���û�е���ִ��ʱ�䣬ת����һ��
        pDispatcherInfo->userThreadQueue.moveToNext();
        pCurrentUserThreadInfo = pDispatcherInfo->userThreadQueue.current();
    }
    //������NULL����ҪNULL���
    if (pCurrentUserThreadInfo != NULL)
        RestoreContext(&pCurrentUserThreadInfo->runContext);
    infoSet.delInfo(currentThreadId);
    return TRUE;
}
