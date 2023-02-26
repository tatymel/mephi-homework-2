

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
        TListNode* next_ = nullptr;
        TListNode* prev_ = nullptr;
        T value_;
        mutable std::shared_mutex mutex_;
        TListNode() = default;
        TListNode(const T& val) :  value_(std::move(val)){}

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
            std::unique_lock uniqueLock(current_->mutex_);
            return current_->value_;
        }

        T operator *() const {
            std::shared_lock uniqueLock(current_->mutex_);
            return current_->value_;
        }

        T* operator ->() {
            std::unique_lock uniqueLock(current_->mutex_);
            return &(current_->value_);
        }

        const T* operator ->() const {
            std::shared_lock uniqueLock(current_->mutex_);
            return &(current_->value_);
        }

        Iterator& operator ++() {
            if(current_->next_ != nullptr)
                std::lock_guard sharedLock1(current_->next_->mutex_);
            std::lock_guard uniqueLock(current_->mutex_);

            current_ = current_->next_;
            return *this;
        }

        Iterator operator ++(int) {

            Iterator old = *this;
            //std::lock_guard<std::mutex> uniqueLock(old.current_->mutex_);
            ++(*this);
            return old;
        }

        Iterator& operator --() {
            if(current_->prev_ != nullptr)
                std::lock_guard sharedLock1(current_->prev_->mutex_);

            std::lock_guard uniqueLock(current_->mutex_);
            current_ = current_->prev_;
            return *this;
        }

        Iterator operator --(int) {
            //std::lock_guard<std::mutex> uniqueLock(current_->mutex_);
            Iterator old = *this;
            //std::lock_guard<std::mutex> uniqueLock(old.current_->mutex_);
            --(*this);
            return old;
        }

        bool operator ==(const Iterator& rhs) const {
            //std::lock_guard uniqueLock(current_->mutex_);
            return rhs.current_ == current_;
        }

        bool operator !=(const Iterator& rhs) const {
            std::lock_guard uniqueLock(current_->mutex_);
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
    Iterator begin() const {
        std::lock_guard<std::mutex> uniqueLock(mutex_);
        return Iterator(head_);
    }

    /*
     * Получить итератор, указывающий на "элемент после последнего" элемента в списке
     */
    Iterator end() const {
        std::lock_guard<std::mutex> uniqueLock(mutex_);
        return Iterator(end_);
    }

    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T& value) {
        std::lock_guard<std::mutex> uniqueLock(mutex_);
        TListNode* newNode = new TListNode(value);

        if(head_ == nullptr){
            head_ = newNode;
            tail_ = head_;
            end_->prev_ = tail_;
            tail_->next_ = end_;
        }else{
            TListNode* it = position.GetCurrent();
            if (it == nullptr) {
                tail_->next_ = newNode;
                newNode->prev_ = tail_;
                tail_ = newNode;
                end_->prev_ = tail_;
                tail_->next_ = end_;
            } else if (it == head_) {

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
        std::lock_guard<std::mutex> uniqueLock(mutex_);
        TListNode* it = position.GetCurrent();

        if(it != nullptr) {
            it->prev_->next_ = it->next_;
            it->next_->prev_ = it->prev_;

            it->next_ = nullptr;
            it->prev_ = nullptr;
        }
    }
private:
    TListNode* head_ = nullptr;
    TListNode* tail_ = nullptr;
    TListNode* end_ = nullptr;
    mutable std::mutex mutex_;
};