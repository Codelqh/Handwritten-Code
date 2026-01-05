// #ifdef __CONTROLBLOCK_h__
// #define __CONTROLBLOCK_h__

#include <atomic>
// 控制块实现要点
/*
    1. 内部维护一个强引用和弱引用计数,用原子变量；
    2. 一个备用指针；
    3. 四个接口，增加/删除共享指针的引用计数；增加/删除弱引用计数;
    4. 在弱引用计数为0时候删除控制块本身，在共享对象引用计数为0的时候删除对象。
*/

template<typename T>
class ControlBlock{
public:
    std::atomic<long> shared_count{1};
    std::atomic<long> weak_count{1};
    T* ptr;
    ControlBlock(T* p) : ptr(p) {}

    void inc_shared() {++shared_count;}
    void dec_shared() {
        if(--shared_count == 0){
            delete ptr;
        }
        if(weak_count.load() == 0){
            delete this;
        }
    }

    void inc_weak() {++weak_count;}
    void dec_weak() {
        if(--weak_count == 0 && shared_count.load() == 0){
            delete this;
        }
    }
    
};

// #endif