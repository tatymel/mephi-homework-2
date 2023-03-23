#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <chrono>

/*
 * Требуется написать класс ThreadPool, реализующий пул потоков, которые выполняют задачи из общей очереди.
 * С помощью метода PushTask можно положить новую задачу в очередь
 * С помощью метода Terminate можно завершить работу пула потоков.
 * Если в метод Terminate передать флаг wait = true,
 *  то пул подождет, пока потоки разберут все оставшиеся задачи в очереди, и только после этого завершит работу потоков.
 * Если передать wait = false, то все невыполненные на момент вызова Terminate задачи, которые остались в очереди,
 *  никогда не будут выполнены.
 * После вызова Terminate в поток нельзя добавить новые задачи.
 * Метод IsActive позволяет узнать, работает ли пул потоков. Т.е. можно ли подать ему на выполнение новые задачи.
 * Метод GetQueueSize позволяет узнать, сколько задач на данный момент ожидают своей очереди на выполнение.
 * При создании нового объекта ThreadPool в аргументах конструктора указывается количество потоков в пуле. Эти потоки
 *  сразу создаются конструктором.
 * Задачей может являться любой callable-объект, обернутый в std::function<void()>.
 */

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount){

        isActive_.store(true);
        isTerminate_.store(false);
        threads_.reserve(threadCount);

        for(size_t i = 0; i < threadCount; ++i){
            threads_.emplace_back([&]{
                while(true) {
                    if (!IsActive()) {
                        break;
                    }

                    {
                        std::unique_lock lock(mutex_);
                        if (!IsActive()) {
                            break;
                        }

                        if (!tasks_.empty()) {

                            std::function<void()> func = std::move(tasks_.front());
                            tasks_.pop();

                            lock.unlock();
                            func();
                        }else{
                            if(isTerminate_){
                                break;
                            }
                        }
                    }

                }

                {
                    std::unique_lock lock(mutex_);
                    while (!tasks_.empty()) {
                        tasks_.pop();
                    }
                }
            });
        }
    }

    void PushTask(const std::function<void()>& task) {
        std::lock_guard lock(mutex_);
        if(!isTerminate_.load()) {
            tasks_.push(task);
        }else{
            throw std::exception();
        }
    }

    void Terminate(bool wait) {
        std::unique_lock lock(mutex_);
        isTerminate_.store(true);
        if(!wait){
            isActive_ = false;
        }
        //std::cout << "tyt 96" << std::endl;
        lock.unlock();

        for(auto& thread : threads_){
            thread.join();
        }

    }

    bool IsActive() const {
        return isActive_.load();
    }

    size_t QueueSize() const {
        std::unique_lock lock(mutex_);
        return tasks_.size();
    }
private:
    std::atomic<bool> isActive_;
    std::atomic<bool> isTerminate_;

    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;

    mutable std::mutex mutex_;
};