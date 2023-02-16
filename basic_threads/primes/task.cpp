

#include <cstdint>
#include <mutex>
#include <set>
#include <atomic>
#include <vector>

/*
 * Класс PrimeNumbersSet -- множество простых чисел в каком-то диапазоне
 */
class PrimeNumbersSet {
public:
    PrimeNumbersSet() = default;

    // Проверка, что данное число присутствует в множестве простых чисел
    bool IsPrime(uint64_t number) const{
        std::lock_guard<std::mutex> lo(set_mutex_);
        return primes_.find(number) != primes_.end();
    }

    // Получить следующее по величине простое число из множества
    uint64_t GetNextPrime(uint64_t number) const{
        std::lock_guard<std::mutex> lo(set_mutex_);
        auto it = primes_.upper_bound(number);
        if( it != primes_.end()){
            return *it;
        }else{
            throw std::invalid_argument("error");
        }
    }

    void AddPrimesInRange(uint64_t from, uint64_t to){


        for(uint64_t i = from; i < to; ++i){
            bool fl = true;
            for(uint64_t j = 2; j * j <= i; ++j){
                if(i%j == 0){
                    fl = false;
                    break;
                }
            }
            if(fl && i != 1 && i != 0){
                const std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
                set_mutex_.lock();
                const std::chrono::time_point<std::chrono::high_resolution_clock> finish = std::chrono::high_resolution_clock::now();
                nanoseconds_waiting_mutex_.fetch_add(std::chrono::duration<uint64_t, std::nano>(finish - start).count());
                primes_.insert(i);
                set_mutex_.unlock();
                const std::chrono::time_point<std::chrono::high_resolution_clock> finish2 = std::chrono::high_resolution_clock::now();
                nanoseconds_under_mutex_.fetch_add(std::chrono::duration<uint64_t, std::nano>(finish2 - finish).count());

            }
        }


    }

    // Посчитать количество простых чисел в диапазоне [from, to)
    size_t GetPrimesCountInRange(uint64_t from, uint64_t to) const{
        std::lock_guard<std::mutex> lo(set_mutex_);
        auto itStart = primes_.lower_bound(from);
        auto itEnd = primes_.lower_bound(to);
        size_t ans = 0;
        for(auto it = itStart; it != itEnd; ++it){
            ++ans;
        }
        if(itEnd != primes_.end() && *itEnd == to){ --ans; }
        return ans;
    }

    // Получить наибольшее простое число из множества
    uint64_t GetMaxPrimeNumber() const{
        std::lock_guard<std::mutex> lo(set_mutex_);
        return *primes_.rbegin();
    }

    // Получить суммарное время, проведенное в ожидании лока мьютекса во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeWaitingForMutex() const{
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::nanoseconds(nanoseconds_waiting_mutex_));
    }

    // Получить суммарное время, проведенное в коде под локом во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeUnderMutex() const{
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::nanoseconds((nanoseconds_under_mutex_)));
    }
private:
    std::set<uint64_t> primes_;
    mutable std::mutex set_mutex_;
    std::atomic<uint64_t> nanoseconds_under_mutex_, nanoseconds_waiting_mutex_;
};