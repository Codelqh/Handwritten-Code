#include <memory>     // 包含 std::default_delete
#include <utility>    // 包含 std::move, std::forward
#include <type_traits> // 包含 std::remove_reference

//独占智能指针实现要点
/*
    1. 独占所有权（不可拷贝，可移动）
    2. 自定义删除器（支持函数对象、lambda、函数指针）
    3. 完整的运算符重载（*, ->, bool, 比较等）
    4. release(), reset(), swap() 等接口
*/
template<typename T, typename Deleter = std::default_delete<T>>
class UniquePtr{
public:
/*********************构造与析构***********************/
    //0.构造
    explicit UniquePtr(T* p = nullptr) : ptr_(p), deleter_() {}
    UniquePtr(T* p, const Deleter& d) : ptr_(p), deleter_(d) {}
    UniquePtr(T* p, Deleter&& d) : ptr_(p), deleter_(std::move(d)) {}

    //1.禁用拷贝和拷贝赋值
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    //2. 移动构造
    UniquePtr(UniquePtr&& other) noexcept: 
        ptr_(other.ptr_), 
        deleter_(std::move(other.deleter_)) 
    {
        other.ptr_ = nullptr;
    }

    //3. 移动赋值
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            reset(); // 先清理自己
            ptr_ = other.ptr_;
            deleter_ = std::forward<Deleter>(other.deleter_); // ← 转移删除器
            other.ptr_ = nullptr;
        }
        return *this;
    }

    //4. 析构
    ~UniquePtr() {reset();}

/********************运算符重载**********************/   
    T* operator->() const {return ptr_;}
    T& operator*() const {return *ptr_;}
    explicit operator bool() const{
        return ptr_ != nullptr;
    }

/********************接口**********************/   
    void reset(T* p = nullptr) noexcept {
        if (ptr_ != p) {
            if (ptr_) deleter_(ptr_);
            ptr_ = p;
        }
    }

    T* release(){
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }

private:
    T* ptr_;
    Deleter deleter_;
};