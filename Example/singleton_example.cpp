#include "Singleton.h"
#include <iostream>

using namespace std;
class ExampleSingleton : public Singleton<ExampleSingleton> {
    friend class Singleton<ExampleSingleton>;
public:
    void showMessage() {
        cout << "Hello from Singleton!" << endl;
    }
private:
    ExampleSingleton() = default;
    
};  

int main() {
    ExampleSingleton& instance = Singleton<ExampleSingleton>::getInstance();
    instance.showMessage();
    return 0;
}