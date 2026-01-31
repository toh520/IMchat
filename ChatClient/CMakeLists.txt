cmake_minimum_required(VERSION 3.16)

# 项目名称
project(ChatClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启自动化特性
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ==========================================
# 强制使用静态运行时 (/MT) 以匹配 Protobuf 库
# ==========================================
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    
    # 手动替换 /MD 为 /MT
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

find_package(Qt6 COMPONENTS Widgets Network REQUIRED)


# ==========================================
# 手动指定 Protobuf 路径 (B计划)
# ==========================================
set(PROTOBUF_ROOT "D:/tools/protobufpeizhi/protobuf-install")
include_directories(${PROTOBUF_ROOT}/include)
link_directories(${PROTOBUF_ROOT}/lib)

# 查找 protoc 编译器
set(PROTOC_EXE "${PROTOBUF_ROOT}/bin/protoc.exe")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 编译 Proto 文件
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/msg.proto")
set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/msg.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/msg.pb.h")

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${PROTOC_EXE}
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "Running protoc compiler..."
    VERBATIM
)

# 源文件列表
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    chatwindow.cpp
    chatwindow.h
    usermanager.cpp
    usermanager.h
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

if(WIN32)
    add_executable(ChatClient WIN32 ${PROJECT_SOURCES})
else()
    add_executable(ChatClient ${PROJECT_SOURCES})
endif()

# 链接库
target_link_libraries(ChatClient PRIVATE Qt6::Widgets Qt6::Network)

# 手动链接 Protobuf 库
# Windows MSVC 下通常名为 libprotobuf.lib
target_link_libraries(ChatClient PRIVATE libprotobuf)

# 在构建目录下生成 compile_commands.json (便于VSCode智能感知)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
