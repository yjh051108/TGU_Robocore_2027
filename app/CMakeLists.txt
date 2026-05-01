cmake_minimum_required(VERSION 3.16)

add_library(auto_aim STATIC
    auto_aim/auto_aim.cpp
    auto_aim/armor.cpp
    auto_aim/classifier.cpp
    auto_aim/detector.cpp
)

target_include_directories(auto_aim PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(auto_aim PUBLIC
    io
    tools
    Eigen3::Eigen
    openvino::runtime
    fmt::fmt
)
