//
// Created by Park on 24. 8. 22.
//
#ifndef MODERNOBJECTPOOL_H
#define MODERNOBJECTPOOL_H

#include <Types.h>
#include <vector>
#include <memory>

#include <iostream>

template<typename T>
class ModernObjectPool {
    using storageType = std::aligned_storage_t<sizeof(T),alignof(T)>;
    using storagePtr = std::unique_ptr<storageType>;

public:
    static void Deleter(T* obj, ModernObjectPool<T>* pool) {
        pool->Free(obj);
    }

    explicit ModernObjectPool(size_t initialSize = 100)
    {
        expandPool(initialSize);
    }

    ~ModernObjectPool() = default;

    void Free(T* obj)
    {
        std::destroy_at(obj);
        _pool.emplace_back(reinterpret_cast<storageType*>(obj));
    }

    template <typename ...Args>
    PoolPtr<T> Alloc(Args... args) {
        if (_pool.empty()) {
            expandPool(_pool.capacity()+1);
        }
        // 풀에서 객체를 가져옴
        T* objPtr = reinterpret_cast<T*>(_pool.back().release());
        _pool.pop_back();
        std::construct_at(objPtr, std::forward<Args>(args)...);

        return PoolPtr<T>(
            objPtr,
            [this](T* ptr) { Deleter(ptr, this); }
        );
    }
    auto Size() const
    {
        return _pool.size();
    }

    template<typename Base>
    PoolPtr<Base>
    Cast(PoolPtr<T> ptr) {
        static_assert(std::is_base_of_v<Base,T>,"Invalid Type");

        Base* basePtr = static_cast<Base*>(ptr.release());
        return PoolPtr<Base>(
            basePtr,
            [this](Base* basePtr) { Deleter(static_cast<T*>(basePtr), this); }
        );
    }

private:

    void expandPool(size_t count)
    {
        for(int i = 0; i< count ; ++i)
        {
            _pool.emplace_back(new storageType);
        }
    }
    std::vector<storagePtr> _pool;
};



#endif //MODERNOBJECTPOOL_H
