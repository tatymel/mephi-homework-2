

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

        Iterator(TListNode* cur) : current_(std::move(cur)){
            //iterMutex_.lock();
        }
        Iterator(const Iterator& other) : current_(std::move(other.current_)){}

        T& operator *() {
            current_->mutex_.lock();
            return current_->value_;
        }

        T operator *() const {
            std::shared_lock uniqueLock(current_->mutex_);
            return current_->value_;
        }

        T* operator ->() {
            std::shared_lock uniqueLock(current_->mutex_);
            return &(current_->value_);
        }

        const T* operator ->() const {
            std::shared_lock uniqueLock(current_->mutex_);
            return &(current_->value_);
        }

        Iterator& operator ++() {
            current_->mutex_.unlock();
            if(current_ != nullptr)
                std::lock_guard uniqueLock(current_->mutex_);

            if(current_->next_ != nullptr)
                std::lock_guard sharedLock1(current_->next_->mutex_);


            current_ = current_->next_;

            return *this;
        }

        Iterator operator ++(int) {
            current_->mutex_.unlock();
            Iterator old = *this;
            //std::lock_guard<std::mutex> uniqueLock(old.current_->mutex_);
            ++(*this);
            return old;
        }

        Iterator& operator --() {
            current_->mutex_.unlock();
            if(current_ != nullptr)
                std::lock_guard uniqueLock(current_->mutex_);

            if(current_->prev_ != nullptr)
                std::lock_guard sharedLock1(current_->prev_->mutex_);


            current_ = current_->prev_;
            return *this;
        }

        Iterator operator --(int) {
            current_->mutex_.unlock();
            Iterator old = *this;
            //std::lock_guard<std::mutex> uniqueLock(old.current_->mutex_);
            --(*this);
            return old;
        }

        bool operator ==(const Iterator& rhs) const {
            std::lock_guard l(iterMutex_);
            return rhs.current_ == current_;
        }

        bool operator !=(const Iterator& rhs) const {
            std::lock_guard l(iterMutex_);
            return (rhs.current_ != current_);
        }

        TListNode* GetCurrent() const{
            std::lock_guard l(current_->mutex_);
            return current_;
        }
        //~Iterator(){current_->mutex_.unlock();}
    private:
        TListNode* current_;
        mutable std::mutex iterMutex_;
    };

    /*
     * Получить итератор, указывающий на первый элемент списка
     */
    Iterator begin() const {
        if(head_ != nullptr)
            std::lock_guard lock(head_->mutex_);
        return Iterator(head_);
    }

    /*
     * Получить итератор, указывающий на "элемент после последнего" элемента в списке
     */
    Iterator end() const {
        std::lock_guard lock(end_->mutex_);
        return Iterator(end_);
    }

    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T& value) {

        TListNode* newNode = new TListNode(value);


        if(head_ == nullptr){
            std::lock_guard lEnd(end_->mutex_);

            head_ = newNode;
            std::lock_guard lHead(head_->mutex_);

            tail_ = head_;

            end_->prev_ = tail_;
            tail_->next_ = end_;
        }else{
            TListNode* it = position.GetCurrent();
            //std::lock_guard lock1(it->mutex_);
            /*if(it->prev_ != nullptr)
                std::lock_guard lock2(it->prev_->mutex_);
            if(it->next_ != nullptr)
                std::lock_guard lock3(it->next_->mutex_);*/

            if (it == end_) {
                std::lock_guard lTail(tail_->mutex_);
                std::lock_guard lEnd(end_->mutex_);

                newNode->prev_ = tail_;
                newNode->next_ = end_;

                end_->prev_ = newNode;
                tail_->next_ = newNode;
                tail_ = newNode;
            } else if (it == head_) {
                std::lock_guard lHead(head_->mutex_);

                newNode->next_ = head_;

                head_->prev_ = newNode;
                head_ = newNode;
            } else {

                std::lock_guard l1(it->mutex_);
                std::lock_guard l2(it->prev_->mutex_);

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
        TListNode* it = position.GetCurrent();
        //std::lock_guard lock1(it->mutex_);

        if(it != nullptr) {
            if(it == head_){
                std::lock_guard lHead(head_->mutex_);
                std::lock_guard l(head_->next_->mutex_);

                head_ = head_->next_;
                head_->prev_ = nullptr;
            }else if(it == tail_){
                std::lock_guard lTail(tail_->mutex_);
                std::lock_guard l(tail_->prev_->mutex_);
                std::lock_guard l2(tail_->next_->mutex_);

                tail_ = tail_->prev_;
                tail_->next_ = end_;
                end_->prev_ = tail_;
            }else {
                std::lock_guard l1(it->mutex_);
                std::lock_guard l2(it->prev_->mutex_);
                std::lock_guard l3(it->next_->mutex_);

                it->prev_->next_ = it->next_;
                it->next_->prev_ = it->prev_;


                it->next_ = nullptr;
                it->prev_ = nullptr;
            }
        }
    }
private:
    TListNode* head_ = nullptr;
    TListNode* tail_ = nullptr;
    TListNode* end_ = nullptr;
    //mutable std::mutex mutex_;
};