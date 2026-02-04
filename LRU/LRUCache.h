// lrucache.h
#pragma once
#include <cassert>
#include <list>
#include <unordered_map>
#include <functional>
#include <memory>
#include <utility>

// 默认删除器：自动处理指针类型（调用 delete）和非指针类型（无操作）
template<typename T>
struct DefaultValueDeleter {
    void operator()(T& value) {
        if constexpr (std::is_pointer_v<T>) {
            // 对于指针，调用delete
            delete value;
            value = nullptr;
        }
    }
};

// LRUCache 实现：基于双列表的引用计数 LRU 缓存
// 核心设计：
// 1. not_use 列表维护 LRU 顺序（头部 = 最久未使用）（ref == 1)
// 2. in_use 列表存储外部正在使用的节点（ref > 1）
// 3. to_del 列表存储已移出缓存但外部仍引用的节点（由 HandleGuard 清理）
template<typename KEY, typename VALUE, class ValueDeleter = DefaultValueDeleter<VALUE>>
class LRUCache {
private:
    // 缓存节点结构
    struct Node {
        VALUE value;
        KEY key;
        int ref;            // 引用计数（外部持有 + 缓存自身）
        bool in_cache;      // 是否在缓存中（用于状态判断）
        
        typename std::list<Node>::iterator list_pos;    // 指向当前节点在哪个列表
        
        Node(const KEY& k, VALUE&& v) 
            : value(std::move(v)), key(k), in_cache(false), ref(1) {}
            
        Node(const KEY& k, const VALUE& v) 
            : value(v), key(k), in_cache(false), ref(1) {}
    };

    typedef typename std::list<Node>::iterator ListNodeIterator;

public:
    // RAII 安全句柄：自动管理引用计数
    class HandleGuard {
    public:
        ~HandleGuard() {
            reset();
        }
        
        // 禁用拷贝
        HandleGuard(const HandleGuard&) = delete;
        HandleGuard& operator=(const HandleGuard&) = delete;
        // 禁用移动赋值
        HandleGuard& operator=(HandleGuard&&) = delete;
        
        // 允许移动构造
        HandleGuard(HandleGuard&& other) noexcept
            : cache(other.cache), node(other.node) 
        {
            other.cache = nullptr;
            other.node = nullptr;
        }
        
        // 显式释放资源
        void reset() {
            if (valid()) {
                cache->unref_node(node);
                cache = nullptr;
                node = nullptr;
            }
        }
        
        VALUE& value() const { 
            assert(valid());
            return node->value; 
        }
        
        const KEY& key() const { 
            assert(valid());
            return node->key; 
        }
        
        bool valid() const { 
            return cache && node;
        }
        
        explicit operator bool() const { return valid(); }
        
    private:
        HandleGuard() : cache(nullptr), node(nullptr) {}
        // 只允许 
        explicit HandleGuard(LRUCache* cache, Node* node)
            : cache(cache), node(node) 
        {
            if (cache && node) {
                cache->ref_node(node);
            }
        }
        friend class LRUCache<KEY, VALUE, ValueDeleter>;
        LRUCache* cache;
        Node* node;
    };

public:
    // 构造函数：初始化缓存
    LRUCache() : max_size(0), size(0) {}
    
    // 析构函数：确保所有资源被安全释放
    ~LRUCache()
    {
        // 如果外部仍在使用 LRUCache，则报错
        assert(in_use.empty());
        assert(to_del.empty());
        // 清理所有缓存
        prune();
    }
    
    // Disable copy and move
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = delete;
    LRUCache& operator=(LRUCache&&) = delete;
    
    // 设置最大容量，0表示无限制
    void set_max_size(size_t max_size)
    {
        this->max_size = max_size;
        evict_if_needed();
    }
    
    size_t get_max_size() const { return max_size; }
    size_t get_size() const { return size; }
    
    // 清除所有未被使用的缓存
    void prune()
    {
        for (auto it = not_use.begin(); it != not_use.end(); ) {
            Node& node = *it;
            
            // 从 map 中移除
            cache_map.erase(node.key);
            
            // 清理节点
            node.in_cache = false;
            value_deleter(node.value);
            
            // erase 返回下一个有效迭代器
            it = not_use.erase(it);
            size--;
        }
    }
    
    // 获取缓存项
    HandleGuard get(const KEY& key)
    {
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            // 节点在缓存中，返回句柄（自动增加引用计数）
            return HandleGuard(this, &(*it->second));
        }
        return HandleGuard();
    }
    
    // 插入缓存项
    template<typename V>
    HandleGuard put(const KEY& key, V&& value)
    {
        static_assert(std::is_constructible_v<VALUE, V&&>, "value 必须用来构造 VALUE");
        auto it = cache_map.find(key);
        
        if (it != cache_map.end()) {
            // 键已存在，更新值
            ListNodeIterator node_iter = it->second;
            node_iter->value = std::forward<V>(value);
            return HandleGuard(this, &(*node_iter));
        }
        
        // 创建新节点，直接放在 in_use 中（因为调用者有引用）
        ListNodeIterator node_iter = not_use.emplace(not_use.end(), key, std::forward<V>(value));
        
        node_iter->in_cache = true;
        node_iter->ref = 1;  // 1 for cache
        node_iter->list_pos = node_iter;
        
        // 添加到缓存映射
        cache_map[key] = node_iter;
        size++;
        
        // 如果需要，进行淘汰
        evict_if_needed();
        
        return HandleGuard(this, &(*node_iter));
    }
    
    // 删除缓存项（从缓存中移除，不释放内存）
    bool del(const KEY& key)
    {
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            ListNodeIterator node_iter = it->second;
            // 从映射中移除
            cache_map.erase(it);
            
            // 从适当的链表中移除
            if (node_iter->ref == 1) {
                // 仅缓存持有：直接从 not_use 移除并清理
                value_deleter(node_iter->value);
                not_use.erase(node_iter);
            } else {
                // 外部持有：移入 to_del（由 HandleGuard 清理）
                to_del.splice(to_del.end(), in_use, node_iter);
                node_iter->in_cache = false;
                node_iter->ref--;   // 释放缓存引用
            }
            
            size--;
            return true;
        }
        return false;
    }

private:
    // 增加节点引用
    void ref_node(Node* node) {
        if (node->in_cache && node->ref == 1) {
            // 从 not_use 移入 in_use（表示节点被外部使用）
            in_use.splice(in_use.end(), not_use, node->list_pos);
        }
        node->ref++;
    }
    
    // 减少节点引用
    void unref_node(Node* node) {
        assert(node->ref > 0);
        node->ref--;
        
        if (node->ref == 0) {
            // 无引用：从 to_del 中移除并清理
            assert(!node->in_cache);
            value_deleter(node->value);
            to_del.erase(node->list_pos);
        } else if (node->in_cache && node->ref == 1) {
            // 从 in_use 移入 not_use（表示节点可被 LRU 淘汰）
            not_use.splice(not_use.end(), in_use, node->list_pos);
        }
    }
    
    // 淘汰检查
    void evict_if_needed() {
        if (max_size > 0) {
            while (size > max_size && !not_use.empty()) {
                auto node_iter = not_use.begin();
                Node& node = *node_iter;
                
                // 从映射中移除
                cache_map.erase(node.key);
                
                // 清理节点
                node.in_cache = false;
                value_deleter(node.value);
                not_use.erase(node_iter);
                size--;
            }
        }
    }

    size_t max_size;
    size_t size;

    std::list<Node> not_use;   // ref=1: 未被外部使用（LRU 队列，头部=最久未使用）
    std::list<Node> in_use;    // ref>1: 外部正在使用（不参与 LRU 淘汰）
    std::list<Node> to_del;    // in_cache=false: 已移出缓存但外部仍引用（由 HandleGuard 清理）
    
    // 键到节点迭代器的映射
    std::unordered_map<KEY, ListNodeIterator> cache_map;
    ValueDeleter value_deleter;
};