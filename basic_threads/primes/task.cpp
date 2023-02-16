//
// Created by Татьяна Мельник on 16.02.2023.
//
#include "task.h"
#include <mutex>
#include <vector>
PrimeNumbersSet::PrimeNumbersSet() = default;
// Проверка, что данное число присутствует в множестве простых чисел
bool PrimeNumbersSet::IsPrime(uint64_t number) const{
    return primes_.find(number) != primes_.end();
}

// Получить следующее по величине простое число из множества
uint64_t PrimeNumbersSet::GetNextPrime(uint64_t number) const{
    auto it = primes_.upper_bound(number);
    if( it != primes_.end()){
        return *it;
    }else{
        throw std::invalid_argument("error");
    }
}

/*
 * Найти простые числа в диапазоне [from, to) и добавить в множество
 * Во время работы этой функции нужно вести учет времени, затраченного на ожидание лока мюьтекса,
 * а также времени, проведенного в секции кода под локом
 */
void PrimeNumbersSet::AddPrimesInRange(uint64_t from, uint64_t to){
    const std::chrono::time_point start1 = std::chrono::high_resolution_clock::now();////maybe steady_clock???
    std::unique_lock<std::mutex> lo{set_mutex_, std::defer_lock};
    lo.lock();
    const std::chrono::time_point start = std::chrono::high_resolution_clock::now();

    std::vector<bool> pr(to, true);
    pr[0] = pr[1] = false;
    for(uint64_t i = 2; i < to; ++i){
        if(pr[i]){
            if(i >= from)
                primes_.insert(i);
            for(uint64_t j = i * i; j < to; j += i){
                pr[j] = false;
            }
        }
    }


    const std::chrono::time_point finish = std::chrono::high_resolution_clock::now();
    nanoseconds_under_mutex_.fetch_add(std::chrono::duration<uint64_t, std::nano>(finish - start).count());
    nanoseconds_waiting_mutex_.fetch_add(std::chrono::duration<uint64_t, std::nano>(finish - start1).count());
    lo.unlock();
}

// Посчитать количество простых чисел в диапазоне [from, to)
size_t PrimeNumbersSet::GetPrimesCountInRange(uint64_t from, uint64_t to) const{
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
    return *primes_.rbegin();
}

// Получить суммарное время, проведенное в ожидании лока мьютекса во время работы функции AddPrimesInRange
std::chrono::nanoseconds PrimeNumbersSet::GetTotalTimeWaitingForMutex() const{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::nanoseconds(nanoseconds_waiting_mutex_));
}

// Получить суммарное время, проведенное в коде под локом во время работы функции AddPrimesInRange
std::chrono::nanoseconds PrimeNumbersSet::GetTotalTimeUnderMutex() const{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::nanoseconds((nanoseconds_under_mutex_)));
    //return std::chrono::nanoseconds((nanoseconds_waiting_mutex_));
}
