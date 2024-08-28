cmake_minimum_required(VERSION 3.28)
project(NetLib)
add_compile_options(/D_UNICODE /DUNICODE)

set(CMAKE_CXX_STANDARD 20)
if(MSVC)
    add_compile_options(-translateInclude)
endif()

add_library(NetLib STATIC
        src/CCpuUsage.cpp
        src/CLogger.cpp
        src/CoreGlobal.cpp
        src/CrashDump.cpp
        src/CRecvBuffer.cpp
        src/CRingBuffer.cpp
        src/CSendBuffer.cpp
        src/DBConnection.cpp
        src/Executable.cpp
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
        src/stdafx.cpp
        src/SubnetManager.cpp
        src/TLSLockFreeQueue.cpp
        src/TLSLockFreeQueue2.cpp
        src/TLSLockFreeStack.cpp
        src/TLSPool.cpp
        src/WebHook.cpp
        ../../packetParser/FVector.cpp
        src/Utility.cpp
        include/ModernObjectPool.h
        include/ModernObjectPool.h
        include/ModernObjectPool.h)

target_include_directories(NetLib PUBLIC include)
target_include_directories(NetLib PUBLIC ../../packetParser)
target_include_directories(NetLib PRIVATE include/externalHeader)

file(GLOB EXTERNAL_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/Libs/*.lib")

foreach(lib ${EXTERNAL_LIBS})
    target_link_libraries(NetLib PUBLIC ${lib})
endforeach()