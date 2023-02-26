

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
class ThreadSafeList {
public:
    /*
     * Класс-итератор, позволяющий обращаться к элементам списка без необходимости использовать мьютекс.
     * При этом должен гарантироваться эксклюзивный доступ потока, в котором был создан итератор, к данным, на которые
     * он указывает.
     * Итератор, созданный в одном потоке, нельзя использовать в другом.
     */
    ThreadSafeList(): end_(new TListNode()){}
    struct TListNode{
        TListNode* next_;
        TListNode* prev_;
        T value_;
        std::mutex mutex_;
        TListNode() = default;
        TListNode(const T& val) : next_(nullptr), prev_(nullptr), value_(std::move(val)){}
        //TListNode(TListNode* next,TListNode* prev, const T& val) : next_(std::move(next)), prev_(std::move(prev)), value_(std::move(val)){}
    };

    class Iterator {
    public:
        using pointer = T*;
        using value_type = T;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator(TListNode* cur) : current_(std::move(cur)){}

        T& operator *() {
            return current_->value_;
        }

        T operator *() const {
            //T val = current_->value_;
            return current_->value_;
        }

        T* operator ->() {
            return &(current_->value_);
        }

        const T* operator ->() const {
            return &(current_->value_);
        }

        Iterator& operator ++() {
            current_ = current_->next_;
            return *this;
        }

        Iterator operator ++(int) {
            Iterator old = *this;
            ++(*this);
            return old;
        }

        Iterator& operator --() {

            current_ = current_->prev_;
            return *this;
        }

        Iterator operator --(int) {
            Iterator old = *this;
            --(*this);
            return old;
        }

        bool operator ==(const Iterator& rhs) const {
            return rhs.current_ == current_;
        }

        bool operator !=(const Iterator& rhs) const {
            return !(rhs == *this);
        }

        TListNode* GetCurrent() const{
            return current_;
        }
    private:
        TListNode* current_;
    };

    /*
     * Получить итератор, указывающий на первый элемент списка
     */
    Iterator begin() {
        std::unique_lock<std::mutex> uniqueLock(mutex_);
        return Iterator(head_);
    }

    /*
     * Получить итератор, указывающий на "элемент после последнего" элемента в списке
     */
    Iterator end() {
        std::unique_lock<std::mutex> uniqueLock(mutex_);
        return Iterator(end_);
    }

    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T& value) {
        std::unique_lock<std::mutex> uniqueLock(mutex_);

        TListNode* newNode = new TListNode(value);

        if(head_ == nullptr){
            head_ = newNode;
            tail_ = head_;
            end_->prev_ = tail_;
        }else{


            TListNode* it = position.GetCurrent();
            if (it == nullptr) {

                tail_->next_ = newNode;
                newNode->prev_ = tail_;
                tail_ = newNode;
                end_->prev_ = tail_;
            } else if (it == head_) {

                std::lock_guard lockk(it->mutex_);
                std::lock_guard lock1(head_->mutex_);
                std::lock_guard lock(newNode->mutex_);

                newNode->next_ = head_;
                head_->prev_ = newNode;
                head_ = newNode;

            } else {

                newNode->prev_ = it->prev_;
                newNode->next_ = it;

                it->prev_->next_ = newNode;
                it->prev_ = newNode;
            }

        }
    }

    /*
     * Стереть из списка элемент, на который указывает итератор `position`
     */
    void erase(Iterator position) {
        std::unique_lock<std::mutex> uniqueLock(mutex_);
        TListNode* it = position.GetCurrent();

        it->prev_->next_ = it->next_;
        it->next_->prev_ = it->prev_;

        it->next_ = nullptr;
        it->prev_ = nullptr;
    }
private:
    TListNode* head_ = nullptr;
    TListNode* tail_ = nullptr;
    TListNode* end_ = nullptr;
    std::mutex mutex_;
};