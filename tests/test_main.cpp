//
// Created by Park on 24. 8. 28.
//

#include <gtest/gtest.h>
#include "ModernObjectPoolTest.h"
#include "MultiThreadObjectPoolTest.h"


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}