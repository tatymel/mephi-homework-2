#include <mutex>
#include <condition_variable>
#include <optional>
#include <atomic>
#include <iostream>

class TimeOut : public std::exception {
    const char* what() const noexcept override {
        return "Timeout";
    }
};


template<typename T>
class UnbufferedChannel {
public:
    void Put(const T& data) {
        std::unique_lock<std::mutex> lock(mutex_);
        while(!hasReadElemnt_){
            canPut_.wait(lock);
        }
        value_ = data;
        hasReadElemnt_ = false;
        canGet_.notify_one();

        otherThreadRead_ = false;

        while (!otherThreadRead_){
            blockThread_.wait(lock);
        }
    }

    T Get(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        std::unique_lock<std::mutex> lock(mutex_);
        std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

        if(timeout == std::chrono::milliseconds(0)){

            while(hasReadElemnt_){
                canGet_.wait(lock);
            }

            T val = value_;
            hasReadElemnt_ = true;

            otherThreadRead_ = true;
            blockThread_.notify_all();
            canPut_.notify_one();
            return val;
        }else{
            while(hasReadElemnt_){
                if(canGet_.wait_until(lock, start + timeout) == std::cv_status::timeout){
                    break;
                }
            }
            if(hasReadElemnt_){
                otherThreadRead_ = true;
                blockThread_.notify_all();
                canPut_.notify_one();

                throw TimeOut();
            }else{
                T val = value_;
                hasReadElemnt_ = true;

                otherThreadRead_ = true;
                blockThread_.notify_all();
                canPut_.notify_one();
                return val;
            }
        }


    }
private:
    T value_;
    bool hasReadElemnt_ = true;
    bool otherThreadRead_ = false;
    std::mutex mutex_;
    std::condition_variable canPut_;
    std::condition_variable canGet_;
    std::condition_variable blockThread_;
};
