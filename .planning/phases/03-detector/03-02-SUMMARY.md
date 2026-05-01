# Plan 03-02 Summary: 迁移 Detector

**Status:** Complete ✓

## What was done
- 创建 app/auto_aim/detector.hpp（无 PCA 声明）
- 创建 app/auto_aim/detector.cpp（TOML 配置、LOG_DEBUG 日志、保留 fmt/imshow）
- PCA 函数和调用删除（原项目已禁用）
- 保留 YOLO detect 重载、save()、show_result()

## Verification
- 零 yaml-cpp ✓ | 零 lightbar_points_corrector ✓ | 零 tools::logger() ✓
- toml::parse_file config ✓ | LOG_DEBUG 日志 ✓ | fmt::format 保留 ✓
- 两个 detect 重载 ✓ | 所有 private 方法 ✓
- 编译 100% 通过 ✓
