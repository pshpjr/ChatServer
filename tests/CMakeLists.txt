
# 테스트 소스 파일
add_executable(test_main
        test_main.cpp
        ModernObjectPoolTest.h
        MultiThreadObjectPoolTest.h
)

# 헤더 파일 경로 설정
target_include_directories(test_main PRIVATE ../include)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(test_main PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
# 테스트 등록
add_test(NAME MyTest COMMAND test_main)

