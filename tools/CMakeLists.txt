cmake_minimum_required(VERSION 3.16)

add_library(tools STATIC
    crc.cpp
    logger.cpp
    foxglove_comm.cpp
    math_tools.cpp
    img_tools.cpp
)

target_include_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/tools/foxglove/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}/tools/foxglove/lib
)

target_link_libraries(tools PUBLIC
    Eigen3::Eigen
    foxglove
    Boost::boost
    pthread
)
