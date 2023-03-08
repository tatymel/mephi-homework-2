#include <mutex>
#include <condition_variable>
#include <optional>

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
        while(hasNotRead_){
            canPut_.wait(lock);
        }

        value_ = data;
        hasNotRead_ = true;
        canGet_.notify_one();
    }

    T Get(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        std::unique_lock<std::mutex> lock(mutex_);
        std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

        if(timeout == std::chrono::milliseconds(0)){
            while (!hasNotRead_) {
                canGet_.wait(lock);
            }

            T val = value_;
            hasNotRead_ = false;
            canPut_.notify_one();
            return val;
        }else {
            while (!hasNotRead_) {
                std::chrono::time_point<std::chrono::steady_clock> finish = std::chrono::steady_clock::now();
                if(finish - start > timeout) {
                    hasNotRead_ = true;
                }
                canGet_.wait_until(lock, start + timeout);
            }

            std::chrono::time_point<std::chrono::steady_clock> finish = std::chrono::steady_clock::now();

            if(finish - start > timeout) {
                hasNotRead_ = false;
                throw TimeOut();
            }else{
                T val = value_;
                hasNotRead_ = false;
                canPut_.notify_one();
                return val;
            }
        }

    }
private:
    T value_;
    bool hasNotRead_ = false;
    std::mutex mutex_;
    std::condition_variable canPut_;
    std::condition_variable canGet_;

};
