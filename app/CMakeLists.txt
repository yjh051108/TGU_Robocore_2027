cmake_minimum_required(VERSION 3.16)

add_library(auto_aim STATIC
    auto_aim/auto_aim.cpp
    auto_aim/armor.cpp
    auto_aim/classifier.cpp
    auto_aim/detector.cpp
    auto_aim/solver.cpp
    auto_aim/target.cpp
    auto_aim/tracker.cpp
    auto_aim/voter.cpp
    auto_aim/aimer.cpp
    auto_aim/shooter.cpp
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
