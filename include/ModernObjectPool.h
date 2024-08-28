//
// Created by Park on 24. 8. 22.
//
#ifndef MODERNOBJECTPOOL_H
#define MODERNOBJECTPOOL_H

#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <cstdlib>
#include <new>

template<typename T>
class ModernObjectPool {

public:
    static void Deleter(T* obj, ModernObjectPool<T>* pool) {
        pool->Free(obj);
    }

    explicit ModernObjectPool(size_t initialSize = 100) {
        expandPool(initialSize);
    }

    ~ModernObjectPool() = default;

    void Free(T* obj)
    {
        obj->~T();
        pool.emplace_back(reinterpret_cast<storageType*>(obj));
    }

    template <typename ...Args>
    PoolPtr<T> Alloc(Args... args) {
        if (pool.empty()) {
            expandPool(pool.size()+1);
        }
        // 풀에서 객체를 가져옴
        auto objRef = std::move(pool.back());
        pool.pop_back();

        new (objRef.get()) T(std::forward<Args>(args)...);

        return std::unique_ptr<T, std::function<void(T*)>>(
            objRef.release(),
            [this](T* ptr) { Deleter(ptr, this); }
        );
    }


    template<typename Base,typename T>
    PoolPtr<Base>
    Cast(PoolPtr<T>&& ptr) {
        static_assert(std::is_base_of_v<Base,T>(),"Invalid Type");

        Base* basePtr = static_cast<Base*>(ptr.release());
        return PoolPtr<Base>(
            basePtr,
            [this](Base* basePtr) { Deleter(static_cast<T*>(basePtr), this); }
        );
    }

private:
    using storageType = std::aligned_storage_t<sizeof(T),alignof(T)>;
    void expandPool(size_t count) {

        // 해당 메모리 블록에 객체를 생성하고 풀에 추가
        for (size_t i = 0; i < count; ++i) {
            pool.emplace_back(storageType());
        }

    }


    std::vector<std::unique_ptr<storageType>> pool;
};



#endif //MODERNOBJECTPOOL_H
