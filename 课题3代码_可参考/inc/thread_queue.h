#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include "gloghelper.h"
// 线程安全队列
template <typename T>
class SafeQueue {
public:

    // Operator [] to access element
    T& operator[](size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= queue_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return queue_[index];
    }

    // Const version of operator [] to access element
    const T& operator[](size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= queue_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return queue_[index];
    }

    void push(const T& value) 
    {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(value);
            cond_var_.notify_one();
    }

    void pop()
    {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock, [this] { return !queue_.empty(); });
            queue_.pop_front();
    }

    T front()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        T value = queue_.front();
        return value;
    }

    bool empty() const
    {
        if(queue_.empty())
        {
            return true;
        }

        return false;
    }

    //return size
    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();

    }

    //return begin
    typename std::deque<T>::const_iterator begin() 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.begin();

    }

    // void erase(typename std::deque<T>::iterator fast, typename std::deque<T>::iterator last) 
    // {
    //     std::lock_guard<std::mutex> lock(mutex_);
    //     queue_.erase(fast, last);
    // }

    void erase(int size)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for(int i = 0; i < size; i++)
        {
            queue_.pop_front();
        }
    }

private:
    std::deque<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
};

template <typename T>
class SafeVector
{
public:
// Operator [] to access element
    T& operator[](size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= vector_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return vector_[index];
    }

    // Const version of operator [] to access element
    const T& operator[](size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= vector_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return vector_[index];
    }
    void push(const T& value) 
    {
            std::lock_guard<std::mutex> lock(mutex_);
            vector_.push_back(value);
            // cond_var_.notify_one();
    }

    //return size
    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return vector_.size();
    }
    bool empty() const
    {
        if(vector_.empty())
        {
            return true;
        }

        return false;
    }

    void copyfrom(std::vector<T>& vec)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto num : vec)
        {
            vector_.push_back(num);
        }
    }
    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        vector_.clear();
    }
    typename std::vector<T>::iterator begin() {
        std::lock_guard<std::mutex> lock(mutex_);
        return vector_.begin();
    }

    typename std::vector<T>::iterator end() {
        std::lock_guard<std::mutex> lock(mutex_);
        return vector_.end();
    }
    
private:
    std::vector<T> vector_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
};