# Phase 1: 构建系统适配 - Context

**Gathered:** 2026-05-01
**Status:** Ready for planning

<domain>
## Phase Boundary

为自瞄迁移项目搭建编译基础设施：添加缺失的第三方依赖（Eigen3），创建 tools/ 的 CMakeLists.txt，启用 app/ 子目录，修复 io 与 tools 的链接关系，确保项目完整编译通过。

</domain>

<decisions>
## Implementation Decisions

### tools CMake 组织
- **D-01:** `tools/` 编译为 STATIC 库，和 `io` 保持一致
- **D-02:** `tools/CMakeLists.txt` 包含现有源文件：`crc.cpp`、`logger.cpp`、`foxglove_comm.cpp`
- **D-03:** `tools` 链接 `Boost::boost`、`pthread`（foxglove_comm 需要）

### Eigen3 安装方式
- **D-04:** 使用 apt 安装：`sudo apt install libeigen3-dev`
- **D-05:** 取消顶层 CMakeLists.txt 中 `find_package(Eigen3 REQUIRED)` 的注释

### app/ 目录结构
- **D-06:** 先搭框架，不编译具体源文件
- **D-07:** `app/CMakeLists.txt` 先创建一个空的 `auto_aim` 库 target（后续 phase 逐步添加源文件）
- **D-08:** 顶层 CMakeLists.txt 取消 `add_subdirectory(app)` 的注释

### io 与 tools 链接
- **D-09:** 在创建 `tools/CMakeLists.txt` 后，取消 `io/CMakeLists.txt` 中 `tools` 链接的注释
- **D-10:** 链接方式保持 `PUBLIC`，让链接 io 的目标也能自动链接 tools

### Claude's Discretion
- CMake 构建的细节（C++ 标准、编译选项、输出目录等）沿用现有配置即可

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### 项目规范
- `项目结构与代码风格.md` — CMake 组织规范、目录职责、命名风格

### 现有构建配置
- `CMakeLists.txt` — 顶层 CMake 配置
- `io/CMakeLists.txt` — io 模块 CMake，作为 tools CMake 的参考
- `app/CMakeLists.txt` — app 模块 CMake（需更新）
- `app/auto_aim/auto_aim.hpp` — auto_aim 占位文件

### 代码分析
- `.planning/codebase/CONCERNS.md` — 记录了 tools 缺少 CMakeLists.txt 的问题
- `.planning/codebase/STACK.md` — 技术栈分析

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `io/CMakeLists.txt` — 可直接参考其 STATIC 库写法来创建 `tools/CMakeLists.txt`

### Established Patterns
- 项目使用 `target_include_directories` + `target_link_libraries` 而非全局 `include_directories`
- STATIC 库 + PUBLIC 依赖传递是现有模式

### Integration Points
- 顶层 CMakeLists.txt 已 `add_subdirectory(tools)` 但 tools/ 缺少 CMakeLists.txt — 这是当前编译阻塞点
- `io/CMakeLists.txt` 中 `tools` 链接被注释 — 需 tools target 存在后才能启用

</code_context>

<specifics>
## Specific Ideas

- tools STATIC 库的结构尽量和 io/CMakeLists.txt 保持一致，方便阅读和维护

</specifics>

<deferred>
## Deferred Ideas

None — 讨论范围控制在 Phase 1 的基础设施建设内

</deferred>

---

*Phase: 1-构建系统适配*
*Context gathered: 2026-05-01*
