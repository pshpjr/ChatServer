cmake_minimum_required(VERSION 3.28)
project(NetLib)

#cmake 관련 속성 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/$ENV{USERNAME}/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake")

#컴파일 설정
if(MSVC)
    add_compile_options(-translateInclude)
    add_compile_options(/D_UNICODE /DUNICODE /utf-8)
endif()


#소스 추가
add_library(NetLib STATIC
        src/CCpuUsage.cpp
        src/CLogger.cpp
        src/CoreGlobal.cpp
        src/CrashDump.cpp
        src/CRecvBuffer.cpp
        src/CRingBuffer.cpp
        src/CSendBuffer.cpp
        src/DBConnection.cpp
        src/Group.cpp
        src/GroupExecutable.cpp
        src/GroupManager.cpp
        src/HardwareMonitor.cpp
        src/LockFreeStack.cpp
        src/LockGuard.cpp
        src/LockPool.cpp
        src/LockStack.cpp
        src/MemoryPool.cpp
        src/MemoryUsage.cpp
        src/MultiThreadMemoryPool.cpp
        src/NormalIOCP.cpp
        src/NormalSocket.cpp
        src/PostSendExecutable.cpp
        src/ProcessMonitor.cpp
        src/Profiler.cpp
        src/RecvExecutable.cpp
        src/RedisConnection.cpp
        src/ReleaseExecutable.cpp
        src/RUDPSocket.cpp
        src/SendBuffer.cpp
        src/SendExecutable.cpp
        src/Session.cpp
        src/SettingParser.cpp
        src/SingleThreadObjectPool.cpp
        src/SingleThreadQ.cpp
        src/SpinLock.cpp
        src/SpinLockPool.cpp
        src/SubnetManager.cpp
        src/TLSLockFreeQueue.cpp
        src/TLSLockFreeQueue2.cpp
        src/TLSLockFreeStack.cpp
        src/TLSPool.cpp
        src/Utility.cpp
        src/WebHook.cpp
        src/stdafx.cpp
)

#define 추가
#디버그 매크로 설정
# 디버그 및 릴리스 매크로 설정
if (CMAKE_CONFIGURATION_TYPES)  # Multi-config generators
    target_compile_definitions(NetLib PRIVATE $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:RELEASE>)
else()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(NetLib PUBLIC DEBUG)
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_definitions(NetLib PUBLIC RELEASE)
    endif()
endif()


##### 외부 패키지
find_package(range-v3 REQUIRED)
target_link_libraries(NetLib PUBLIC range-v3)

#인클루드 설정

target_include_directories(NetLib PUBLIC include)
target_include_directories(NetLib PUBLIC include/externalHeader)

#라이브러리 링킹
file(GLOB EXTERNAL_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.lib")
foreach(lib ${EXTERNAL_LIBS})
    target_link_libraries(NetLib PUBLIC ${lib})
endforeach()

target_link_libraries(NetLib PUBLIC Pdh.lib)
target_link_libraries(NetLib PUBLIC Synchronization.lib)
target_link_libraries(NetLib PUBLIC Winmm.lib)
target_link_libraries(NetLib PUBLIC Ws2_32.lib)
target_link_libraries(NetLib PUBLIC Dbghelp.lib)
target_link_libraries(NetLib PUBLIC Psapi.lib)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    file(GLOB EXTERNAL_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/debug/*.lib")
    foreach(lib ${EXTERNAL_LIBS})
        target_link_libraries(NetLib PUBLIC ${lib})
    endforeach()
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    file(GLOB EXTERNAL_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/*.lib")
    foreach(lib ${EXTERNAL_LIBS})
        target_link_libraries(NetLib PUBLIC ${lib})
    endforeach()
endif()

#출력 파일 이름과 경로 설정
set_target_properties(NetLib PROPERTIES
        OUTPUT_NAME_DEBUG NetLib_d
        OUTPUT_NAME_RELEASE NetLib
)

enable_testing()
add_subdirectory(tests)