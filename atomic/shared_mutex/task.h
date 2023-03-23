#pragma once

#include <atomic>
#include <iostream>
#include <thread>


class SharedMutex {
public:

    void lock() {
        int temp;
        do{
            temp = 0;
        }while (!Shared_.compare_exchange_strong(temp, -1));
    }

    void unlock() {
        Shared_.store(0);
    }

    void lock_shared() {//-1 - TABOO
        int tempShared;
        do {
            do {
                tempShared = Shared_.load();
            } while (tempShared == -1);

            tempShared = Shared_.load();
        } while (!Shared_.compare_exchange_strong(tempShared, tempShared + 1));
    }

    void unlock_shared() {
        Shared_.fetch_sub(1);
    }
private:
    std::atomic<int> Exclusive_ = 0;
    std::atomic<int> Shared_ = 0;

};