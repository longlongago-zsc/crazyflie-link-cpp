cmake_minimum_required (VERSION 3.5)
project(crazyflie-link-cpp)

# define some options
option(BUILD_PYTHON_BINDINGS "Generate Python Bindings" OFF)
option(BUILD_CPP_EXAMPLES "Generate C++ Examples" ON)

# Enable C++14 and warnings
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  # Avoid min/max macros in Windows.h (pulled in from libusb)
  #add_definitions(-DNOMINMAX)
  # warning level 4
  # Ideally we would also treat warnings as errors (/WX), but there are still some warnings to fix
  #add_compile_options(/W4)
else()
  # lots of warnings and all warnings as errors
  add_compile_options(-Wall -Wextra -Werror)
endif()

include_directories(
  include
)

# C++ library

add_library(crazyflieLinkCpp
  src/UdpManager.cpp
  src/Connection.cpp
  src/CrazyfileUdp.cpp
  src/CrazyfileUdpThread.cpp
  src/BoostUdpWrapper.cpp
  src/Version.cpp
)

# target_include_directories(libCrazyflieLinkCpp
#   PUBLIC
#     $<INSTALL_INTERFACE:include/crazyflie_cpp>
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/crazyflie_cpp>
# )

# Apple macOS libraries
if (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "-lobjc -framework IOKit -framework CoreFoundation")
  set(CMAKE_MODULE_LINKER_FLAGS "-lobjc -framework IOKit -framework CoreFoundation")
endif()

# Link pthread on Linux and Mac only
if (NOT MSVC)
target_link_libraries(crazyflieLinkCpp
  PRIVATE
    pthread
)
endif()

set_property(TARGET crazyflieLinkCpp PROPERTY POSITION_INDEPENDENT_CODE ON)

# C++ example application
if (BUILD_CPP_EXAMPLES)
  # example_console
  add_executable(example_console
    examples/console.cpp
  )
  target_link_libraries(example_console
    crazyflieLinkCpp
  )

  # example_scan
  add_executable(example_scan
    examples/scan.cpp
  )
  target_link_libraries(example_scan
    crazyflieLinkCpp
  )

  # example_benchmark
  add_executable(example_benchmark
    examples/benchmark.cpp
  )
  target_link_libraries(example_benchmark
    crazyflieLinkCpp
  )

  # example_broadcast
  add_executable(example_broadcast
    examples/broadcast.cpp
  )
  target_link_libraries(example_broadcast
    crazyflieLinkCpp
  )

   # example_udpTest
  add_executable(example_udpTest
    examples/udpTest.cpp
  )
  target_link_libraries(example_udpTest
    crazyflieLinkCpp
  )
endif()

# 设置Boost库的根目录（应指向 Boost 的安装根目录）
set(BOOST_ROOT "D:/longlongago/crazyflie-lib-cpp/boost_1_80_0")
# 禁用搜索系统路径
set(Boost_NO_SYSTEM_PATHS ON) # 非常重要，否则会导致找不到 Boost 库
# 查找Boost库，指定需要的组件
# find_package(Boost REQUIRED COMPONENTS filesystem regex)
find_package(Boost REQUIRED)
#include_directories(../boost_1_80_0)

# 输出 Boost 的相关信息
message("Boost version: ${Boost_VERSION}")
message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message("Boost_LIBRARIES: ${Boost_LIBRARIES}")

# 包含Boost头文件目录
include_directories(${Boost_INCLUDE_DIRS})

# 链接Boost库目录
link_directories(${Boost_LIBRARY_DIRS})

#link_directories (../boost_1_80_0/stage/lib)
target_link_libraries(crazyflieLinkCpp
    "${BOOST_ROOT}/stage/lib/libboost_serialization-vc143-mt-x64-1_80.lib"
    "${BOOST_ROOT}/stage/lib/libboost_thread-vc143-mt-x64-1_80.lib"
    "${BOOST_ROOT}/stage/lib/libboost_chrono-vc143-mt-x64-1_80.lib"
)

# 设置全局运行时库为多线程非调试版本
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "Runtime Library" FORCE)

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE RelWithDebInfo )

#这个必须放在project的后面
IF(MSVC)
   message("run here")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()
