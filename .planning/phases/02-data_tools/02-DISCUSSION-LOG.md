# Phase 2: 数据结构与工具层 - Discussion Log

> **Audit trail only.** Decisions are captured in CONTEXT.md.

**Date:** 2026-05-01
**Phase:** 2-数据结构与工具层
**Areas discussed:** 目录位置, 日志适配, Include 方式, math_tools 组织

---

## 目录位置

| Option | Description | Selected |
|--------|-------------|----------|
| 按模块分开放 | armor → app/auto_aim/，math/img → tools/ | ✓ |
| 全放 app/auto_aim/ | 所有文件全放 app/auto_aim/ | |
| math/img 放 tools | armor 和 math 放 app，img 放 tools | |

**User's choice:** 按模块分开放
**Notes:** 通用工具放 tools，业务数据放 app，符合 Robocore 设计原则

---

## 日志适配

| Option | Description | Selected |
|--------|-------------|----------|
| 适配 Robocore | tools::logger() → LOG_XXX 宏 | ✓ |
| 保留 spdlog | 保持原样，加 spdlog 依赖 | |

**User's choice:** 适配 Robocore
**Notes:** 解释了为什么 Robocore Logger 更好（无额外依赖、风格统一、Release 优化、自动文件行号）

---

## Include 方式

| Option | Description | Selected |
|--------|-------------|----------|
| 完整路径 | #include "app/auto_aim/armor.hpp" | ✓ |
| 简短路径 | #include "armor.hpp" | |

**User's choice:** 完整路径

---

## math_tools 组织

| Option | Description | Selected |
|--------|-------------|----------|
| 一个文件 | 全部函数放一个 math_tools.hpp/.cpp | ✓ |
| 拆成两个 | 角度+时间差、坐标+矩阵分开 | |

**User's choice:** 一个文件
**Notes:** Claude 推荐，理由：源码一致、函数相关、代码量不大

---

## Claude's Discretion

- 函数内部实现细节不做改动
- CMake 集成由已有 tools/CMakeLists.txt 处理

## Deferred Ideas

None
