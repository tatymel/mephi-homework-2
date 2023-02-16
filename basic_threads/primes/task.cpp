

#include "task.h"

PrimeNumbersSet::PrimeNumbersSet() = default;
    // Проверка, что данное число присутствует в множестве простых чисел
    bool PrimeNumbersSet::IsPrime(uint64_t number) const{
        //std::lock_guard<std::mutex> lo(set_mutex_);
        return primes_.find(number) != primes_.end();
    }

    // Получить следующее по величине простое число из множества
    uint64_t PrimeNumbersSet::GetNextPrime(uint64_t number) const{
        //std::lock_guard<std::mutex> lo(set_mutex_);
        auto it = primes_.upper_bound(number);
        if( it != primes_.end()){
            return *it;
        }else{
            throw std::invalid_argument("error");
        }
    }

    void PrimeNumbersSet::AddPrimesInRange(uint64_t from, uint64_t to){


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
                const std::chrono::time_point<std::chrono::high_resolution_clock> finish2 = std::chrono::high_resolution_clock::now();
                nanoseconds_under_mutex_.fetch_add(std::chrono::duration<uint64_t, std::nano>(finish2 - finish).count());

                set_mutex_.unlock();

            }
        }


    }

    // Посчитать количество простых чисел в диапазоне [from, to)
    size_t PrimeNumbersSet::GetPrimesCountInRange(uint64_t from, uint64_t to) const{
        //std::lock_guard<std::mutex> lo(set_mutex_);
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
    uint64_t PrimeNumbersSet::GetMaxPrimeNumber() const{
        //std::lock_guard<std::mutex> lo(set_mutex_);
        return *primes_.rbegin();
    }

    // Получить суммарное время, проведенное в ожидании лока мьютекса во время работы функции AddPrimesInRange
    std::chrono::nanoseconds PrimeNumbersSet::GetTotalTimeWaitingForMutex() const{
        return std::chrono::nanoseconds(nanoseconds_waiting_mutex_);
    }

    // Получить суммарное время, проведенное в коде под локом во время работы функции AddPrimesInRange
    std::chrono::nanoseconds PrimeNumbersSet::GetTotalTimeUnderMutex() const{
        return std::chrono::nanoseconds(nanoseconds_under_mutex_);
    }
