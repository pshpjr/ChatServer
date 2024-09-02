//
// Created by Park on 24. 8. 28.
//

#ifndef MODERNOBJECTPOOLTEST_H
#define MODERNOBJECTPOOLTEST_H

#include <ModernObjectPool.h>

TEST(ModernObjectPool, Init)
{
    ModernObjectPool<int> pool(100);
    EXPECT_TRUE(pool.Size() == 100);
}
TEST(ModernObjectPool, Alloc)
{
    ModernObjectPool<int> pool(1);
    auto data = pool.Alloc(3);
    EXPECT_TRUE(pool.Size() == 0);
}

TEST(ModernObjectPool, Free)
{
    ModernObjectPool<int> pool(100);
    {
        auto data = pool.Alloc(3);
    }
    EXPECT_TRUE(pool.Size() == 100);
}
TEST(ModernObjectPool, MemReuse)
{
    void* fstPtr = nullptr;
    ModernObjectPool<int> pool(100);
    {
        auto data = pool.Alloc(3);
        fstPtr = data.get();
    }
    {
        auto t = pool.Alloc();
        EXPECT_TRUE(fstPtr == t.get());
    }
}

TEST(ModernObjectPool, DupAlloc)
{
    void* fstPtr = nullptr;
    ModernObjectPool<int> pool(1);
    {
        auto t = pool.Alloc();
        auto t2 = pool.Alloc();
        EXPECT_TRUE(pool.Size() == 1);
    }

    EXPECT_TRUE(pool.Size() == 3);
}

#endif //MODERNOBJECTPOOLTEST_H
