#include <iostream>
#include "UniquePtr.h"

// 测试类
class Resource {
public:
    Resource(int val) : value(val) {
        std::cout << "Resource " << value << " created\n";
    }
    
    ~Resource() {
        std::cout << "Resource " << value << " destroyed\n";
    }
    
    void print() const {
        std::cout << "Resource value: " << value << "\n";
    }
    
    int getValue() const { return value; }
    void setValue(int v) { value = v; }
    
private:
    int value;
};

void test_unique_ptr() {
    std::cout << "=== Testing UniquePtr ===\n";
    
    // 创建UniquePtr
    UniquePtr<Resource> ptr1(new Resource(100));
    ptr1->print();
    
    // 解引用
    (*ptr1).setValue(200);
    (*ptr1).print();
    
    // bool转换
    if (ptr1) {
        std::cout << "ptr1 is not null\n";
    }
    
    // 移动语义
    UniquePtr<Resource> ptr2 = std::move(ptr1);
    if (!ptr1) {
        std::cout << "ptr1 is now null after move\n";
    }
    if (ptr2) {
        std::cout << "ptr2 now owns the resource\n";
        ptr2->print();
    }
    
    // reset测试
    ptr2.reset(new Resource(300));
    ptr2->print();
    
    // release测试
    Resource* raw = ptr2.release();
    if (!ptr2) {
        std::cout << "ptr2 released ownership\n";
    }
    delete raw;
    
    std::cout << "=== End of UniquePtr test ===\n";
}

int main() {
    test_unique_ptr();
    return 0;
}