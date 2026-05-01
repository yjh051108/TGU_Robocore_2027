# Phase 2: 数据结构与工具层 - Context

**Gathered:** 2026-05-01
**Status:** Ready for planning

<domain>
## Phase Boundary

将 sp_vision_25 中的装甲板数据结构和通用工具迁移至 Robocore 框架。包括：
1. `armor.hpp/.cpp` — Lightbar、Armor 数据结构、枚举类型
2. `math_tools.hpp/.cpp` — 数学工具函数（角度限制、坐标转换、时间差等）
3. `img_tools.hpp/.cpp` — 图像绘制工具函数

</domain>

<decisions>
## Implementation Decisions

### 文件目录组织
- **D-01:** armor.hpp/.cpp 放在 `app/auto_aim/` 目录
- **D-02:** math_tools.hpp/.cpp 放在 `tools/` 目录（通用数学工具，不绑定自瞄业务）
- **D-03:** img_tools.hpp/.cpp 放在 `tools/` 目录（通用绘制工具）
- **D-04:** math_tools 保持一个文件不拆分，延续 sp_vision_25 的做法

### 日志适配
- **D-05:** 使用 Robocore 的日志宏替换 spdlog 调用
- **D-06:** `tools::logger()->info(...)` → `LOG_INFO(MODULE, ...)`
- **D-07:** `tools::logger()->debug(...)` → `LOG_DEBUG(MODULE, ...)`
- **D-08:** `tools::logger()->warn(...)` → `LOG_WARN(MODULE, ...)`
- **D-09:** 每个 .cpp 文件顶部定义 `static constexpr const char* MODULE`
- **D-10:** 不引入 spdlog 依赖

### 命名空间
- **D-11:** armor 使用 `namespace app::auto_aim`（遵循 Robocore 项目结构与代码风格.md）
- **D-12:** math_tools 使用 `namespace tools`（与现有 tools 一致）
- **D-13:** img_tools 使用 `namespace tools`（与现有 tools 一致）

### Include 路径
- **D-14:** 使用完整路径 include，例如 `#include "app/auto_aim/armor.hpp"`
- **D-15:** 依赖 CMake 已有的 `target_include_directories(... ${PROJECT_SOURCE_DIR})` 解析

### 代码风格适配
- **D-16:** include guard 格式改为 `TGU_ROBOCORE_2027_APP_AUTO_AIM_ARMOR_HPP`
- **D-17:** 使用 `#pragma once`（与项目现有风格一致）
- **D-18:** 移除 `fmt` 相关依赖（不需要 fmt::format，Detector 阶段处理）
- **D-19:** Eigen3 头文件保持 `<Eigen/Dense>` 和 `<Eigen/Geometry>` 直接引用

### Claude's Discretion
- 函数内部实现细节（算法逻辑）不做改动，保持完整性
- math_tools 和 img_tools 的 CMake 集成由已有的 `tools/CMakeLists.txt` 处理

</decisions>

<canonical_refs>
## Canonical References

### 源项目参考
- `/home/eldwen/sp_vision_25/tasks/auto_aim/armor.hpp` — 装甲板数据结构
- `/home/eldwen/sp_vision_25/tasks/auto_aim/armor.cpp` — 装甲板构造函数实现
- `/home/eldwen/sp_vision_25/tools/math_tools.hpp` — 数学工具接口
- `/home/eldwen/sp_vision_25/tools/math_tools.cpp` — 数学工具实现
- `/home/eldwen/sp_vision_25/tools/img_tools.hpp` — 绘制工具接口
- `/home/eldwen/sp_vision_25/tools/img_tools.cpp` — 绘制工具实现

### 目标框架参考
- `app/auto_aim/auto_aim.hpp` — auto_aim 已有占位头文件（参考 include guard 格式）
- `tools/logger.hpp` — Robocore 日志宏定义
- `项目结构与代码风格.md` — 代码规范

</canonical_refs>

<code_context>
## Existing Code Insights

### 可复用模式
- `app/auto_aim/auto_aim.hpp` 的 include guard 格式可作为 armor.hpp 的参考
- `tools/CMakeLists.txt` 已配置好 tools STATIC 库，添加新源文件即可

### 注意事项
- math_tools 需要链接 `Eigen3::Eigen`（纯头文件库，只需 find_package）
- img_tools 依赖 OpenCV（已在顶层 CMake 中引入）
- armor 依赖 OpenCV 和 Eigen3
- Robocore 项目结构与代码风格.md 要求头文件格式混用 include guard 和 `#pragma once`

</code_context>

<specifics>
## Specific Ideas

- Phase 2 不搬 detector、classifier 等需要图像输入的代码
- Phase 2 不搬 solver（PnP 解算）
- 此阶段只搬数据结构和工具函数

</specifics>

<deferred>
## Deferred Ideas

None

</deferred>

---
*Phase: 2-data_tools*
*Context gathered: 2026-05-01*
