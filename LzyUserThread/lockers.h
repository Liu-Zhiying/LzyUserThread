#ifndef LOCKERS_H
#define LOCKERS_H

#include <atomic>
#include <Windows.h>

//线程不可冲入互斥量，基于原子操作
class single_lock_mutex
{
    std::atomic<bool> _locker;

public:
    single_lock_mutex() : _locker(0) {}
    //阻塞抢锁
    void lock()
    {
        bool expected = false;
        while (!_locker.compare_exchange_strong(expected, true))
        {
            expected = false;
            continue;
        }
    }
    //尝试抢锁
    bool trylock()
    {
        bool expected = false;
        return _locker.compare_exchange_strong(expected, true);
    }
    //解锁
    void unlock()
    {
        bool expected = true;
        _locker.compare_exchange_strong(expected, false);
    }
};

//线程可重入互斥量，基于原子操??
class thread_own_mutex
{
    std::atomic<unsigned int> _count;
    std::atomic<DWORD> _thread_id;

public:
    thread_own_mutex() : _count(0), _thread_id(-1) {}
    // 阻塞抢锁
    void lock()
    {
        DWORD cur_thread_id = GetCurrentThreadId();
        DWORD expected_thread_id = -1;
        DWORD lock_own_thread_id = -1;
        do
        {

            _thread_id.compare_exchange_strong(expected_thread_id, cur_thread_id);
            lock_own_thread_id = expected_thread_id;
            expected_thread_id = -1;
        } while (lock_own_thread_id != expected_thread_id &&
                 lock_own_thread_id != cur_thread_id);
        _count.fetch_add(1);
    }
    // 尝试锁定
    bool trylock()
    {
        DWORD cur_thread_id = GetCurrentThreadId();
        DWORD lock_own_thread_id = -1;
        _thread_id.compare_exchange_strong(lock_own_thread_id, cur_thread_id);
        if (lock_own_thread_id == -1 ||
            lock_own_thread_id == cur_thread_id)
        {
            _count.fetch_add(1);
            return true;
        }
        else
        {
            return false;
        }
    }
    // 解锁
    void unlock()
    {
        DWORD cur_thread_id = GetCurrentThreadId();
        if (_thread_id.compare_exchange_strong(cur_thread_id, cur_thread_id))
        {
            if (_count.fetch_sub(1) == 1)
                _thread_id.compare_exchange_strong(cur_thread_id, -1);
        }
    }
};

//读写锁，线程不可重入
class read_write_lock
{
    std::atomic<unsigned int> _count;
    std::atomic<unsigned int> _id;
public:
    read_write_lock() : _count(0), _id(0) {}
    // 阻塞加读锁
    void read_lock()
    {
        unsigned int target_id = 2;
        unsigned int expected_id = 0;
        unsigned int lock_own_id = 0;
        do
        {
            _id.compare_exchange_strong(expected_id, target_id);
            lock_own_id = expected_id;
            expected_id = 0;
        } while (lock_own_id != expected_id &&
            lock_own_id != target_id);
        _count.fetch_add(1);
    }
    // 尝试加读锁
    bool read_trylock()
    {
        unsigned int cur_id = 2;
        unsigned int lock_own_id = 0;
        _id.compare_exchange_strong(lock_own_id, cur_id);
        if (!lock_own_id || lock_own_id == cur_id)
        {
            _count.fetch_add(1);
            return true;
        }
        else
        {
            return false;
        }
    }
    // 解除读锁
    void read_unlock()
    {
        unsigned int cur_id = 2;
        if (_id.compare_exchange_strong(cur_id, cur_id))
        {
            if (_count.fetch_sub(1) == 1)
                _id.compare_exchange_strong(cur_id, 0);
        }
    }
    // 阻塞加写锁
    void write_lock()
    {
        unsigned int expected_id = 0;
        unsigned int lock_own_id = 0;
        do
        {
            _id.compare_exchange_strong(expected_id, 1);
            lock_own_id = expected_id;
            expected_id = 0;
        } while (lock_own_id != expected_id);
    }
    // 尝试加写锁
    bool write_trylock()
    {
        unsigned int lock_own_id = 0;
        _id.compare_exchange_strong(lock_own_id, 1);
        return !lock_own_id ? true : false;
    }
    // 解除写锁
    void write_unlock()
    {
        unsigned int expect_id = 1;
        _id.compare_exchange_strong(expect_id, 0);
    }
};

#endif