# Plan 02-02 Summary: 创建 math_tools + img_tools

**Status:** Complete ✓

## What was done
- 创建 tools/math_tools.hpp/.cpp（12 个数学函数）
- 创建 tools/img_tools.hpp/.cpp（4 个绘制函数）
- 适配 include guard 和 #pragma once
- 保持 namespace tools 不变

## Verification
- math_tools: guard ✓ | 11 函数 + 1 模板 ✓ | namespace tools ✓
- img_tools: guard ✓ | 4 函数 ✓ | namespace tools ✓
- 全部使用完整 include 路径 ✓
- 零 spdlog/fmt 引用 ✓
