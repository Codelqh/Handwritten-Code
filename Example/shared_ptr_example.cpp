// Example
#include <iostream>
#include "SharePtr.h"

int main() {
    {
        SharePtr<int> sp1(new int(42));
        std::cout << "sp1: " << *sp1 << ", use_count=" << sp1.use_count() << "\n";

        WeakPtr<int> wp(sp1);
        std::cout << "wp.expired(): " << wp.expired() << "\n";

        {
            SharePtr<int> sp2 = wp.lock();
            if (sp2) {
                std::cout << "sp2: " << *sp2 << ", use_count=" << sp2.use_count() << "\n";
            }
        }

        std::cout << "After sp2 destroyed, use_count=" << sp1.use_count() << "\n";

        sp1.reset();
        std::cout << "After sp1 reset, wp.expired(): " << wp.expired() << "\n";
    }

    std::cout << "All done.\n";
    return 0;
}