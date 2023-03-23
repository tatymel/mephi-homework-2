#pragma once
#include <atomic>
#include <iostream>
#include <thread>


class SharedMutex {
public:

    void lock() {
        int v = 0;
        while (!Shared_.compare_exchange_weak(v, -1, std::memory_order::acquire)){
            //std::this_thread::sleep_for(200ms);
        }
    }

    void unlock() {
        Shared_.store(0, std::memory_order::release);
    }

    void lock_shared() {//-1 - TABOO
        int val;
        while(val == -1){
            val = Shared_.load();
        }
        Shared_.compare_exchange_weak(val, val + 1, std::memory_order::acquire);
    }

    void unlock_shared() {
        Shared_.fetch_sub(1, std::memory_order::release);
    }
private:
    std::atomic<bool> IsExclusive_ = false;
    std::atomic<int> Shared_ = 0;

    std::atomic<bool> likeMutexForLoad_;

};