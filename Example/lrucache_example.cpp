// lrucache_test.cpp
#include "LRUCache.h"
#include <iostream>
#include <cassert>
#include <string>
#include <memory>

// 简单的值删除器，用于int类型
struct IntDeleter {
    void operator()(int value) {
        // 对于int，我们什么都不做，但可以添加日志
        // std::cout << "Deleting int: " << value << std::endl;
    }
};

// 用于测试的复杂类型
class TestObject {
public:
    TestObject(int id, const std::string& name) : id(id), name(name) {
        // std::cout << "TestObject created: " << id << ", " << name << std::endl;
    }
    
    ~TestObject() {
        // std::cout << "TestObject destroyed: " << id << ", " << name << std::endl;
    }
    
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    
    bool operator==(const TestObject& other) const {
        return id == other.id && name == other.name;
    }
    
private:
    int id;
    std::string name;
};

// TestObject删除器
struct TestObjectDeleter {
    void operator()(TestObject& obj) {
        // std::cout << "TestObject deleter called: " << obj.getId() << std::endl;
    }
};

void test_basic_operations() {
    std::cout << "=== Test 1: Basic Operations ===" << std::endl;
    
    LRUCache<int, int, IntDeleter> cache;
    cache.set_max_size(10);

    std::cout << "cache size:" << sizeof(cache) << std::endl;
    
    // Test put and get
    {
        auto handle = cache.put(1, 100);
        assert(handle.valid());
        assert(handle.key() == 1);
        assert(handle.value() == 100);
    } // handle goes out of scope, ref count decreases
    
    {
        auto handle = cache.get(1);
        assert(handle.valid());
        assert(handle.value() == 100);
    }
    
    // Test update existing key
    {
        auto handle = cache.put(1, 200);
        assert(handle.valid());
        assert(handle.value() == 200);
    }
    
    {
        auto handle = cache.get(1);
        assert(handle.valid());
        assert(handle.value() == 200);
    }
    
    // Test non-existent key
    {
        auto handle = cache.get(999);
        assert(!handle.valid());
    }
    cache.prune();
    std::cout << "Test 1 passed!" << std::endl;
}

void test_lru_eviction() {
    std::cout << "\n=== Test 2: LRU Eviction ===" << std::endl;
    
    LRUCache<int, int, IntDeleter> cache;
    cache.set_max_size(3);
    
    // Insert 3 items
    {
        cache.put(1, 100);
        cache.put(2, 200);
        cache.put(3, 300);
    }
    
    assert(cache.get_size() == 3);
    
    // Access key 1 to make it recently used
    {
        auto handle = cache.get(1);
        assert(handle.valid());
    }
    
    // Insert a 4th item - should evict key 2 (least recently used)
    {
        cache.put(4, 400);
    }
    
    assert(cache.get_size() == 3);
    
    // Key 2 should be evicted
    {
        auto handle = cache.get(2);
        assert(!handle.valid());
    }
    
    // Keys 1, 3, 4 should still exist
    {
        auto handle1 = cache.get(1);
        auto handle3 = cache.get(3);
        auto handle4 = cache.get(4);
        assert(handle1.valid());
        assert(handle3.valid());
        assert(handle4.valid());
    }
    
    std::cout << "Test 2 passed!" << std::endl;
}

void test_delete_operation() {
    std::cout << "\n=== Test 3: Delete Operation ===" << std::endl;
    
    LRUCache<std::string, TestObject, TestObjectDeleter> cache;
    cache.set_max_size(5);
    
    // Insert some items
    cache.put("obj1", TestObject(1, "Object1"));
    cache.put("obj2", TestObject(2, "Object2"));
    cache.put("obj3", TestObject(3, "Object3"));
    
    assert(cache.get_size() == 3);
    
    // Delete an item
    bool deleted = cache.del("obj2");
    assert(deleted);
    assert(cache.get_size() == 2);
    
    {
        auto handle = cache.get("obj2");
        assert(!handle.valid());
    }
    
    // Try to delete non-existent item
    deleted = cache.del("non-existent");
    assert(!deleted);
    
    std::cout << "Test 3 passed!" << std::endl;
}

void test_handleguard_raii() {
    std::cout << "\n=== Test 4: HandleGuard RAII ===" << std::endl;
    
    LRUCache<int, int, IntDeleter> cache;
    cache.set_max_size(5);
    
    // Put an item
    {
        auto handle = cache.put(1, 100);
        assert(handle.valid());
        assert(cache.get_size() == 1);
        
        // While handle exists, the node should be in in_use list
        // After handle goes out of scope, it moves to not_use
    }
    
    // Handle is destroyed, item should still be in cache but in not_use
    assert(cache.get_size() == 1);
    
    {
        auto handle = cache.get(1);
        assert(handle.valid());
    }
    
    std::cout << "Test 4 passed!" << std::endl;
}

void test_ref_counting() {
    std::cout << "\n=== Test 5: Reference Counting ===" << std::endl;
    
    LRUCache<int, int, IntDeleter> cache;
    cache.set_max_size(5);
    
    // Scenario 1: Delete while handle exists
    {
        auto handle = cache.put(1, 100);
        assert(handle.valid());
        
        // Delete while handle is still alive
        bool deleted = cache.del(1);
        assert(deleted);
        
        // Cache size should decrease
        assert(cache.get_size() == 0);
        
        // But handle should still be valid until destroyed
        assert(handle.valid());
        assert(handle.value() == 100);
    }
    // Now handle is destroyed, node should be deleted
    
    // Scenario 2: Multiple handles
    {
        auto handle1 = cache.put(2, 200);
        assert(handle1.valid());
        
        // Get another handle to same key
        auto handle2 = cache.get(2);
        assert(handle2.valid());
        
        // Delete from cache
        bool deleted = cache.del(2);
        assert(deleted);
        assert(cache.get_size() == 0);
        
        // Both handles should still be valid
        assert(handle1.valid());
        assert(handle2.valid());
        assert(handle1.value() == 200);
        assert(handle2.value() == 200);
    }
    // Both handles destroyed, node should be deleted
    
    std::cout << "Test 5 passed!" << std::endl;
}

void test_move_semantics() {
    std::cout << "\n=== Test 6: Move Semantics ===" << std::endl;
    
    LRUCache<int, int, IntDeleter> cache;
    cache.set_max_size(5);
    
    // Test move constructor of HandleGuard
    {
        auto handle1 = cache.put(1, 100);
        assert(handle1.valid());
        
        // Move construct handle2 from handle1
        auto handle2 = std::move(handle1);
        assert(!handle1.valid());  // handle1 should be invalid after move
        assert(handle2.valid());   // handle2 should be valid
        
        // Use handle2
        assert(handle2.value() == 100);
        
        // handle2 will be destroyed at end of scope
    }
    
    // Test that we can still get the item
    {
        auto handle = cache.get(1);
        assert(handle.valid());
        assert(handle.value() == 100);
    }
    
    std::cout << "Test 6 passed!" << std::endl;
}

void test_prune_operation() {
    std::cout << "\n=== Test 7: Prune Operation ===" << std::endl;
    
    LRUCache<int, int, IntDeleter> cache;
    cache.set_max_size(10);
    
    // Insert some items
    cache.put(1, 100);
    cache.put(2, 200);
    cache.put(3, 300);
    
    assert(cache.get_size() == 3);
    
    // Create a handle to item 2 to keep it referenced
    {
        auto handle = cache.get(2);
        assert(handle.valid());
        
        // Prune should only remove unreferenced items (1 and 3)
        cache.prune();
        
        // Size should be 1 (only item 2 remains)
        assert(cache.get_size() == 1);
        
        // Item 2 should still exist
        assert(handle.valid());
        assert(handle.value() == 200);
    }
    
    // After handle is destroyed, item 2 becomes unreferenced
    cache.prune();
    assert(cache.get_size() == 0);
    
    std::cout << "Test 7 passed!" << std::endl;
}

void test_edge_cases() {
    std::cout << "\n=== Test 8: Edge Cases ===" << std::endl;
    
    // Test with max_size = 0 (unlimited)
    {
        LRUCache<int, int, IntDeleter> cache;
        cache.set_max_size(0);  // Unlimited
        
        // Insert many items
        for (int i = 0; i < 1000; i++) {
            cache.put(i, i * 10);
        }
        
        assert(cache.get_size() == 1000);
        
        // All items should be accessible
        for (int i = 0; i < 1000; i++) {
            auto handle = cache.get(i);
            assert(handle.valid());
            assert(handle.value() == i * 10);
        }
    }
    
    // Test with max_size = 1
    {
        LRUCache<int, int, IntDeleter> cache;
        cache.set_max_size(1);
        
        cache.put(1, 100);
        assert(cache.get_size() == 1);
        
        cache.put(2, 200);  // Should evict item 1
        assert(cache.get_size() == 1);
        
        {
            auto handle1 = cache.get(1);
            assert(!handle1.valid());
            
            auto handle2 = cache.get(2);
            assert(handle2.valid());
            assert(handle2.value() == 200);
        }
    }
    
    std::cout << "Test 8 passed!" << std::endl;
}

void test_complex_types() {
    std::cout << "\n=== Test 9: Complex Types ===" << std::endl;
    
    LRUCache<std::string, std::shared_ptr<TestObject>, 
        std::function<void(std::shared_ptr<TestObject>&)>> cache;
    
    // Use a custom deleter that does nothing (shared_ptr handles destruction)
    auto deleter = [](std::shared_ptr<TestObject>&) {
        // Nothing to do, shared_ptr will handle destruction
    };
    
    // We need to set the deleter - in this case we need to modify LRUCache
    // to accept deleter in constructor. Since our current implementation
    // uses default-constructed deleter, we'll create a wrapper
    
    // For this test, let's use a simpler approach with raw pointer
    // but with a proper deleter
    
    struct SharedPtrDeleter {
        void operator()(std::shared_ptr<TestObject>&) {
            // shared_ptr handles its own destruction
        }
    };
    
    LRUCache<std::string, std::shared_ptr<TestObject>, SharedPtrDeleter> cache2;
    cache2.set_max_size(3);
    
    auto obj1 = std::make_shared<TestObject>(1, "Alice");
    auto obj2 = std::make_shared<TestObject>(2, "Bob");
    auto obj3 = std::make_shared<TestObject>(3, "Charlie");
    
    cache2.put("alice", obj1);
    cache2.put("bob", obj2);
    cache2.put("charlie", obj3);
    
    {
        auto handle = cache2.get("alice");
        assert(handle.valid());
        auto obj = handle.value();
        assert(obj->getId() == 1);
        assert(obj->getName() == "Alice");
    }
    
    std::cout << "Test 9 passed!" << std::endl;
}

int main() {
    try {
        test_basic_operations();
        test_lru_eviction();
        test_delete_operation();
        test_handleguard_raii();
        test_ref_counting();
        test_move_semantics();
        test_prune_operation();
        test_edge_cases();
        test_complex_types();
        
        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}