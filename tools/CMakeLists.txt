cmake_minimum_required(VERSION 3.16)

add_library(tools STATIC
    crc.cpp
    logger.cpp
    foxglove_comm.cpp
)

target_include_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/tools/foxglove/include
)

target_link_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}/tools/foxglove/lib
)

target_link_libraries(tools PUBLIC
    foxglove
    Boost::boost
    pthread
)
