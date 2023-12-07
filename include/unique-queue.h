#pragma once

#include <mutex>
#include <condition_variable>
#include <list>
#include <unordered_map>
#include <atomic>

#define Container std::list

// Thread safe queue with unique elements id.
// Complexity O(1) for all operations.
// Type T should contain method .getId()
template <typename T>
class UniqueQueue{
public:
    enum class EC{
        inserted,
        overload,
        alreadyInQueue,
        reassigned
    };
    UniqueQueue();

    EC push(const T & t);
    EC push(T && t);

    bool erase(const decltype(std::declval<T>().getId()) id);
    bool tryPop(T & t);
    T pop();

    T top() const;
    bool isInQueue(const decltype(std::declval<T>().getId()) id) const;
    bool isEmpty() const;
    
    bool setMaxSize(const size_t size);
    size_t getMaxSize() const;
    size_t getSize() const;
    void setRejectRepeated(bool);
    bool getRejectRepeated() const;

private:
    Container<T> queue;
    std::atomic<size_t> maxSize;
    bool rejectRepeated;
    std::unordered_map<decltype(std::declval<T>().getId()), 
                       typename Container<T>::iterator> inQueue;
    mutable std::condition_variable checkQueue;
    mutable std::mutex mtx;
};

template <typename T>
inline UniqueQueue<T>::UniqueQueue() :
    rejectRepeated{true}
{}

template <typename T>
bool UniqueQueue<T>::isInQueue(const decltype(std::declval<T>().getId()) id) const{
    std::unique_lock<std::mutex> lck(mtx);
    return inQueue.find(id) != inQueue.end();
}

// If new max queue size < current queue size -> calls will not be droped 
template <typename T>
bool UniqueQueue<T>::setMaxSize(size_t size){
    if (size < 1)
        return false;
    maxSize = size;
    return true;
}

template <typename T>
inline size_t UniqueQueue<T>::getMaxSize() const{
    return maxSize;
}

template <typename T>
inline bool UniqueQueue<T>::isEmpty() const{
    std::unique_lock<std::mutex> lck(mtx);
    return queue.empty();
}

template <typename T>
inline size_t UniqueQueue<T>::getSize() const{
    std::unique_lock<std::mutex> lck(mtx);
    return queue.size();
}

template <typename T>
inline void UniqueQueue<T>::setRejectRepeated(bool rejectRepeated){
    this->rejectRepeated = rejectRepeated;
}

template <typename T>
inline bool UniqueQueue<T>::getRejectRepeated() const{
    return rejectRepeated;
}

template <typename T>
typename UniqueQueue<T>::EC UniqueQueue<T>::push(T && t){
    std::unique_lock<std::mutex> lck(mtx);
    if (queue.size() >= maxSize)
        return UniqueQueue<T>::EC::overload;
    auto id = t.getId();
    auto found = inQueue.find(id);
    bool repeated = false;
    if (found != inQueue.end()){
        if (rejectRepeated)
            return UniqueQueue<T>::EC::alreadyInQueue;
        queue.erase(found->second);
        inQueue.erase(found);
        repeated = true;
    }
    queue.push_back(std::move(t));
    auto iter = queue.rbegin();
    inQueue[id] = (++iter).base();
    checkQueue.notify_one();
    return repeated? UniqueQueue<T>::EC::reassigned :
                     UniqueQueue<T>::EC::inserted;
}

template <typename T>
inline typename UniqueQueue<T>::EC UniqueQueue<T>::push(const T &t){
    auto tCopy = t;
    return push(std::move(tCopy));
}

// Blocking pop
template <typename T>
T UniqueQueue<T>::pop(){
    std::unique_lock<std::mutex> lck(mtx);
    while (queue.size() <= 0)
        checkQueue.wait(lck);
    auto t = queue.front();
    queue.pop_front();
    inQueue.erase(t.getId());
    return t;
}

template <typename T>
inline T UniqueQueue<T>::top() const{
    std::unique_lock<std::mutex> lck(mtx);
    return queue.front();
}

template <typename T>
inline bool UniqueQueue<T>::erase(const decltype(std::declval<T>().getId()) id){
    std::unique_lock<std::mutex> lck(mtx);
    auto t = inQueue.find(id);
    if (t == inQueue.end())
        return false;
    queue.erase(t->second);
    inQueue.erase(t);
    return true;
}

template <typename T>
inline bool UniqueQueue<T>::tryPop(T & t)
{
    std::unique_lock<std::mutex> lck(mtx);
    while (queue.size() <= 0)
        return false;
    t = queue.front();
    queue.pop_front();
    inQueue.erase(t.getId());
    return true;
}

#undef Container