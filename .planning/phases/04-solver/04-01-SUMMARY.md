# Plan 04-01 Summary: 迁移 Solver (PnP 解算)

**Status:** Complete ✓

## What was done
- 创建 app/auto_aim/solver.hpp（去掉 outpost/SJTU 声明，加 yaw_search_range 成员）
- 创建 app/auto_aim/solver.cpp（TOML 配置、删掉 outpost/SJTU 函数体、可配 yaw_search_range）
- 添加 solver.cpp 到 CMake

## Verification
- yaml-cpp: 0 引用 ✓
- oupost_reprojection_error: 已删除 ✓
- SJTU_cost: 已删除 ✓
- yaw_search_range: 可配置 ✓
- toml::parse_file config 读取 ✓
- 编译 100% 通过 ✓
