#include "ThreadPool.h"
#include <iostream>

int main(){
    ThreadPool pool(4);

    auto future1 = pool.Enqueue([](int a, int b){
        std::cout << "Calculating sum of " << a << " and " << b << std::endl;
        return a + b;
    }, 5, 10);

    auto future2 = pool.Enqueue([](const std::string& str){
        std::cout << "Appending ' World!' to " << str << std::endl;
        return str + " World!";
    }, "Hello");

    int result1 = future1.get();
    std::string result2 = future2.get();
    return 0;
}