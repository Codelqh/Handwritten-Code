#ifndef SHAREPTR_H
#define SHAREPTR_H

#include "ControlBlock.h"

//共享智能指针实现要点
/*
    1. 内部维护一个对象指针和控制块指针；
    2. 构造函数，拷贝构造函数，移动构造函数;
       拷贝赋值，移动赋值重载；
       指针的引用和解引用重载；
    3. 需提供的接口：
        get()：返回shared_ptr中保存的裸指针；
        use_count()：返回shared_ptr的强引用计数；
        reset()：重置shared_ptr；
        unique():若智能指针强引用计数为1的时候返回true，反之返回false；
*/

template<typename T>
class WeakPtr; // 前向声明

template<typename T>
class SharePtr{
    template<typename U> friend class WeakPtr;
public:
/************************一、构造函数和析构函数************************/
    //1.构造：从原始指针
    explicit SharePtr(T* p = nullptr):
        ptr_(p),
        block_(p ? new ControlBlock<T>(p) : nullptr)
    {

    }

    //2.拷贝构造
    SharePtr(const SharePtr& other) : 
        ptr_(other.ptr_),
        block_(other.block_)
    {
        if(block_) block_->inc_shared();
    }

    //3.移动构造
    SharePtr(SharePtr&& other) noexcept: 
        ptr_(other.ptr_),
        block_(other.block_)
    {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    ~SharePtr(){
        reset();
    }
/************************二、运算符重载************************/
//赋值运算符
    //拷贝赋值
    SharePtr& operator=(const SharePtr& other){
        if(this != &other){
            reset();
            block_ = other.block_;
            ptr_ = other.ptr_;
            if(block_){
                block_->inc_shared();
            }
        }
        return *this;
    }

    //移动赋值
    SharePtr& operator=(SharePtr&& other) noexcept{
        if(this != &other){
            reset();
            block_ = other.block_;
            ptr_ = other.ptr_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
        return *this;
    }


    explicit operator bool() const {
        return ptr_ != nullptr;
    }
    // 解引用
    T& operator*() const {return *ptr_;}
    T* operator->() const {return ptr_;}

/************************三、需对外提供的接口************************/
    void reset(){
        if(block_){
            block_->dec_shared();
            block_ = nullptr;
        }
    }

    T* get() const {return ptr_;}

    long use_count() const {
        return block_ ? block_->shared_count.load() : 0;
    }

    bool unique() {
        return use_count() == 1;
    }
private:
    T* ptr_;
    ControlBlock<T>* block_;
private:
    SharePtr(ControlBlock<T>* block): 
        ptr_(block ? block->ptr : nullptr), 
        block_(block) 
    {
        if (block_) block_->inc_shared();
    }
};


//弱引用指针实现要点
/*
    1. 构造函数：
        从一个共享智能指针开始创建；
    2. 对外接口：
        use_count():返回强引用计数
        expired():对象是否还有效,如果无效了返回true
        lock():提供一个临时的share_ptr对象给外界访问
    3. SharePtr需将WeakPtr设置为友元类，方便访问私有变量
*/
template<typename T>
class WeakPtr{
public:
/************************一、构造函数和析构函数************************/
    //1. 默认构造
    WeakPtr() = default;

    //2. 从SharePtr构造
    WeakPtr(const SharePtr<T>& sp):
        block_(sp.block_)
    {
        if(block_) block_->inc_weak();
    }

    //3. 拷贝构造
    WeakPtr(const WeakPtr<T>& wp):
        block_(wp.block_)
    {
        if(block_) block_->inc_weak();
    }

    ~WeakPtr(){
        reset();
    }
/************************二、运算符重载************************/
    //1. 拷贝赋值
    WeakPtr& operator=(const WeakPtr<T>& wp){
        if(this != &wp){
            reset();
            block_ = wp.block_;
            if(block_) block_->inc_weak();
        }
        return *this;
    }

/************************三、需对外提供的接口************************/

    void reset(){
        if(block_){
            block_->dec_weak();
            block_ = nullptr;
        }
    }

    long use_count(){
        return block_ ? block_->shared_count.load() : 0;
    }

    bool expired(){
        return use_count() == 0;
    }

    SharePtr<T> lock(){
        if(expired()) { return SharePtr<T>();}
        return SharePtr<T>(block_);
    }

private:
    ControlBlock<T>* block_{nullptr};
};
#endif