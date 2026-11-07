#ifndef NANO_EDR_RING_BUFFER_H
#define NANO_EDR_RING_BUFFER_H

#include <cstddef>
#include <array>

#include "event.h"

namespace nano_edr {

template <typename T, std::size_t N>
class RingBuffer {
 public:
    class Iterator {
     public:
        Iterator(const T* p, size_t index, size_t remaining) : p_(p), index_(index), remaining_(remaining) {}
        const T& operator*() const {
            return p_[index_];
        }
        Iterator& operator++(){
            index_ = (index_ + 1) % N;
            remaining_--;
            return *this;
        }
        bool operator!=(const Iterator& other) const{
            return remaining_ != other.remaining_;
        }

    private:
        const T* p_ = nullptr;
        size_t index_ = 0;
        size_t remaining_ = 0;
    };

    void PushBack(const T& value){
        c_[index_] = value;
        index_ = (index_ + 1) % N;
        if (index_ == 0 ){
            is_full_ = true;
        }
    }

    void Clear(){
        index_ = 0;
        is_full_ = false;
        c_ = {};
    }

    std::size_t size() const {
        if (is_full_){
            return N;
        }
        return index_;
    }

    static constexpr std::size_t capacity(){
        return N;
    }
    bool empty() const{
        return size() == 0;
    }
    bool full() const{
        return is_full_;
    }

    const T& operator[](std::size_t index) const {
        if (is_full_) {
        return c_[(index_ + index) % N];
        }
        return c_[index];
    }

    const T& back() const {
        if (index_ == 0){
            return c_[N - 1];
        }
        return c_[(index_ + N - 1) % N];
    }

    Iterator begin() const { 
        if (is_full_){
            return Iterator(c_.data(), index_, N);
        }
        return Iterator(c_.data(), 0, index_);
    }

    Iterator end() const {
        return Iterator(c_.data(), 0, 0);
    }

 private:
    std::size_t index_ = 0;
    bool is_full_ = false;

    std::array<T, N> c_;
};

}

#endif