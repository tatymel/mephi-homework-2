#pragma once
#include <atomic>
#include <iostream>
#include <thread>


class SharedMutex {
public:

    void lock() {
        while (IsExclusive_.load() && Shared_.load() > 0){
            //std::this_thread::sleep_for(200ms);
        }
        IsExclusive_.store(true);
    }

    void unlock() {
        if(IsExclusive_.load())
            IsExclusive_.store(false);
    }

    void lock_shared() {
        while(IsExclusive_.load())
        {
            //std::this_thread::sleep_for(200ms);
        }
        Shared_.fetch_add(1);
    }

    void unlock_shared() {
        if(Shared_.load() > 0){
            Shared_.fetch_add(-1);
        }
    }
private:
    std::atomic<bool> IsExclusive_ = false;
    std::atomic<int> Shared_ = 0;
};