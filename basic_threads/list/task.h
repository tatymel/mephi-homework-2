#pragma once

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <atomic>
#include <vector>
#include <iostream>

/*
 * Потокобезопасный связанный список.
 */
template<typename T>
struct Node{

    Node* next = nullptr;
    Node* prev = nullptr;
    T value;
    std::mutex mutex_;
    std::atomic<bool> uses = false;
    Node() = default;
    Node(Node<T>* n, Node<T>* p, const T& val) : next(n), prev(p), value(std::move(val)){}
};
template<typename T>
class ThreadSafeList {
public:
    /*
     * Класс-итератор, позволяющий обращаться к элементам списка без необходимости использовать мьютекс.
     * При этом должен гарантироваться эксклюзивный доступ потока, в котором был создан итератор, к данным, на которые
     * он указывает.
     * Итератор, созданный в одном потоке, нельзя использовать в другом.
     */
    class Iterator {
    public:
        using pointer = T*;
        using value_type = T;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator(Node<T>* pos) : current_(std::move(pos)){}

        T& operator *() {
            //std::cout << "tut 45" << std::endl;
            bool t = false;
            do{
                t = false;
            }while(current_->uses.compare_exchange_strong(t, true));
            std::unique_lock l(current_->mutex_);
            //current_->uses.store(true);
            return current_->value;
        }

        T operator *() const{
            std::unique_lock l(current_->mutex_);
            T val = current_->value;
            return val;
        }

        T* operator ->() {
            std::unique_lock l(current_->mutex_);
            current_->uses.store(true);
            return &current_->value;
        }

        const T* operator ->() const {
            std::unique_lock l(current_->mutex_);
            return &current_->value;
        }

        Iterator& operator ++() {
            std::unique_lock l(current_->mutex_);
            if(current_->next != nullptr) {
                std::unique_lock l1(current_->next->mutex_);

                current_->uses.store(false);
                current_ = current_->next;

            }else{
                current_->uses.store(false);
                current_ = current_->next;
            }

            //current_->mutex_.unlock();
            return *this;
        }

        Iterator operator ++(int) {
            /*  std::unique_lock l(current_->mutex_);
              std::unique_lock l1(current_->next->mutex_);*/
            Iterator t = *this;
            ++(*this);
            return t;
        }

        Iterator& operator --() {
            std::unique_lock l(current_->mutex_);
            if(current_->prev != nullptr) {
                std::unique_lock l1(current_->prev->mutex_);
                current_->uses.store(false);
                current_ = current_->prev;

                //current_->uses.store(true);
            }else{
                current_->uses.store(false);
                current_ = current_->prev;
            }


            //current_->mutex_.unlock();
            return *this;
        }

        Iterator operator --(int) {
            /* std::unique_lock l(current_->mutex_);
             std::unique_lock l1(current_->prev->mutex_);*/
            Iterator t = *this;
            --*this;
            return t;
        }

        bool operator ==(const Iterator& rhs) const {
            if(current_ != nullptr) {
                std::unique_lock l(current_->mutex_);
                //std::cout << "tut 95" << std::endl;
                return (rhs.current_ == current_);
            }else if(rhs.current_ == nullptr){
                return true;
            }
            return false;
        }

        bool operator !=(const Iterator& rhs) const {
            //if(current_ != nullptr) {
            std::unique_lock l(current_->mutex_);
            current_->uses.store(false);

            return (current_ != rhs.current_);
            //}
            if(rhs.current_ != nullptr){
                return true;
            }
            return false;
        }
        Node<T>* getNode()const{
            std::unique_lock l(current_->mutex_);
            return current_;
        };
    private:
        Node<T>* current_;
    };

    /*
     * Получить итератор, указывающий на первый элемент списка
     */
    Iterator begin() {
        std::unique_lock l(allList_);
        //std::cout << "tut 154" << std::endl;
        /* if(head_ != nullptr) {
             //std::unique_lock l1(head_->mutex_);
             head_->uses.store(false);
         }*/
        return Iterator(head_);
    }

    /*
     * Получить итератор, указывающий на "элемент после последнего" элемента в списке
     */
    Iterator end() {
        std::unique_lock l(allList_);
        /* if(tail_ != nullptr) {
             //std::unique_lock l1(tail_->mutex_);
             tail_->uses.store(false);
         }*/
        //std::cout << "tut 127" << std::endl;
        return Iterator(tail_);
    }

    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T& value) {
        std::unique_lock lAll(allList_);
        //std::cout << "tut 114" << std::endl;
        if(head_ == nullptr){//first element
            //std::cout << "tut 116" << std::endl;
            Node<T>* newHead = new Node<T>(nullptr, nullptr, value);
            Node<T>* newTail = new Node<T>(nullptr, nullptr, value);
            newHead->next = newTail;
            newHead->value = value;
            newTail->prev = newHead;
            tail_ = newTail;
            head_ = newHead;
            //std::cout << "tut 124" << std::endl;
        }else{
            //std::cout << "tut 126" << std::endl;
            Node<T>* pos = position.getNode();
            pos = tail_;
            //std::cout << "tut 132" << std::endl;
            std::unique_lock lTemp(pos->mutex_);
            //std::cout << "tut 134" << std::endl;
            std::unique_lock lPrev(pos->prev->mutex_);
            //std::cout << "tut 135" << std::endl;
            lAll.unlock();
            //std::cout << "tut 138" << std::endl;
            Node<T>* newNode = new Node<T>(pos, pos->prev, value);
            //std::cout << "tut 140" << std::endl;
            pos->prev->next = newNode;
            pos->prev = newNode;
            //std::cout << "tut 143" << std::endl;
        }
    }

    /*
     * Стереть из списка элемент, на который указывает итератор `position`
     */
    void erase(Iterator position) {
        std::unique_lock all(allList_);
        if(head_ != nullptr) {
            Node<T>* pos = position.getNode();
            std::unique_lock lT(pos->mutex_);
            std::unique_lock lPrev(pos->prev->mutex_);
            std::unique_lock lNext(pos->next->mutex_);


            all.unlock();

            if (head_->next == tail_) {
                tail_->prev = nullptr;
                head_->next = nullptr;

                head_ = nullptr;
                tail_ = nullptr;
            } else {
                pos->next->prev = pos->prev;
                pos->prev->next = pos->next;
            }
        }
    }

private:
    Node<T>* head_ = nullptr;
    Node<T>* tail_ = nullptr;
    std::mutex headMutex_;
    std::mutex tailMutex_;
    std::mutex allList_;
};