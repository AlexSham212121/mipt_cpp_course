#include "event_list.h"

namespace nano_edr {

EventList::~EventList(){
    Clear();
} 

void EventList::PushBack(const Event& event){
    EventNode* node = new EventNode(event);

    if (head_ == nullptr){
        head_ = node;
        tail_ = node;
    } else {
        tail_->next = node;
        tail_ = node;
    }

    ++size_;

    if (capacity() != 0 && size_ > capacity()){
        PopFront();
    }
}

void EventList::PopFront(){
    if (head_ == nullptr){
        return;
    }

    EventNode* old = head_;
    head_ = head_->next;
    delete old;
    --size_;

    if (head_ == nullptr){
        tail_ = nullptr; 
    }
}

void EventList::Clear(){
    while (head_ != nullptr){
        EventNode* old = head_;
        head_ = head_->next;
        delete old;
    }

    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
}

} 