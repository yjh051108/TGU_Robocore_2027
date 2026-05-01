# Phase 1: 构建系统适配 - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-01
**Phase:** 1-构建系统适配
**Areas discussed:** tools CMake 组织, Eigen3 安装方式, app/ 目录结构, io 与 tools 链接

---

## tools CMake 组织

| Option | Description | Selected |
|--------|-------------|----------|
| STATIC 库 | add_library(tools STATIC ...)，和 io 一致 | ✓ |
| OBJECT 库 | add_library(tools OBJECT ...)，不生成独立 .a 文件 | |

**User's choice:** STATIC 库
**Notes:** 用户不了解 CMake 是什么，解释了 CMake 的基本概念后做选择。

---

## Eigen3 安装方式

| Option | Description | Selected |
|--------|-------------|----------|
| apt 安装 | sudo apt install libeigen3-dev，系统级安装 | ✓ |
| FetchContent | CMake 中自动从 GitHub 下载 | |

**User's choice:** apt 安装
**Notes:** 简单稳定，无需额外配置。

---

## app/ 目录结构

| Option | Description | Selected |
|--------|-------------|----------|
| 先搭框架 | 创建空 auto_aim 库 target，后续逐步加源文件 | ✓ |
| 直接建完整 CMake | 一次性写好所有依赖链接 | |

**User's choice:** 先搭框架
**Notes:** 逐步添加源文件更灵活。

---

## io 与 tools 链接

| Option | Description | Selected |
|--------|-------------|----------|
| 取消注释 tools | io 链接 tools，后续肯定要用 | ✓ |
| 保持注释 | 等用到再加 | |

**User's choice:** 取消注释 tools
**Notes:** 解释了为什么作者注释掉（tools/ 没有 CMakeLists.txt，不存在 tools target 导致 CMake 报错），以及正确的顺序：先建 tools CMake → 再取消注释链接。

---

## Claude's Discretion

- CMake 构建细节（C++ 标准、编译选项等）沿用现有配置

## Deferred Ideas

None
