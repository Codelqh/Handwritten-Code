
#ifndef SINGLETON_H_
#define SINGLETON_H_


/*
    饿汉式单例模式
    实现要点：
        1. 构造函数私有化(用protected)，防止外部创建对象；
        2. 提供一个静态方法获取唯一实例；
        3. 使用局部静态变量，保证线程安全和延迟初始化；
        4. 禁用拷贝构造、移动构造、赋值操作符，防止复制和移动实例。
*/

template<typename T>
class Singleton {
public:
    static T& getInstance() {
        static T instance;
        return instance;
    }
    Singleton(const Singleton&) = delete;
    Singleton(Singleton &&) = delete;
    Singleton& operator=(const Singleton&) = delete; 
    Singleton& operator=(Singleton &&) = delete;

protected:    
    Singleton() = default;
    virtual ~Singleton() = default;
};

#endif // SINGLETON_H_