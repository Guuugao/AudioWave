cmake_minimum_required(VERSION 3.22.1)

# 设置项目名称和版本
project(audioWave VERSION 1.0)

# 设置C标准
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 查找包管理工具
find_package(PkgConfig REQUIRED)
# 查找Alsa库
pkg_check_modules(ALSA REQUIRED alsa)

# 添加源文件
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.c)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接Alsa库
target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARIES})

# 添加包含目录
target_include_directories(${PROJECT_NAME} PRIVATE ${ALSA_INCLUDE_DIRS})

# 设置可执行文件的输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 添加变量定义，以便在代码中使用
ADD_DEFINITIONS(-D_PROJECT_VERSION=\"${CMAKE_PROJECT_VERSION}\")
ADD_DEFINITIONS(-D_PROJECT_NAME=\"${PROJECT_NAME}\")