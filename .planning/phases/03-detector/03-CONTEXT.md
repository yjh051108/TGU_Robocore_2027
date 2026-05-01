# Phase 3: 灯条检测与装甲板匹配 - Context

**Gathered:** 2026-05-01
**Status:** Ready for planning

<domain>
## Phase Boundary

将 sp_vision_25 的 Detector（灯条检测 + 装甲板匹配）和 Classifier（数字分类）完整迁移至 Robocore 框架。包含 YOLO 检测结果的 ROI 精修重载。

</domain>

<decisions>
## Implementation Decisions

### 配置格式
- **D-01:** 将 YAML 配置改为 TOML 格式
- **D-02:** 使用 Robocore 自带的 `tools/tomlpp.hpp` 读取
- **D-03:** 不再依赖 yaml-cpp

### fmt 库
- **D-04:** 保留 fmt 库，不改为 std::format
- **D-05:** `fmt::format()` 调用保持不变
- **D-06:** 在 CMake 中添加 fmt 依赖

### OpenVINO 分类器
- **D-07:** 完整迁移 Classifier，包含 OpenVINO 数字识别
- **D-08:** 安装 OpenVINO（apt 安装）
- **D-09:** `classify()` 和 `ovclassify()` 均迁移

### cv::imshow 调试
- **D-10:** 保留所有 cv::imshow 调用
- **D-11:** 通过 Detector 的 `debug_` 成员控制显示开关

### 日志适配
- **D-12:** `tools::logger()->debug(...)` → `LOG_DEBUG("DETECTOR", ...)`
- **D-13:** 每个 .cpp 定义 `static constexpr const char* MODULE`

### save() 功能
- **D-14:** 保留低置信度装甲板存图功能
- **D-15:** 保存路径由 debug 模式控制

### PCA 角点矫正
- **D-16:** 迁移 lightbar_points_corrector 但保持注释状态（作为兜底）
- **D-17:** 在原项目 commit 583d50a 已禁用，未来如需激活可取消注释

### YOLO detect 重载
- **D-18:** 迁移 `detect(Armor&, const cv::Mat&)` 函数
- **D-19:** 保留 YOLO ROI 精修逻辑，后续接 YOLO 可直接使用

### Claude's Discretion
- Detector 内部的算法逻辑和阈值保持不变

</decisions>

<canonical_refs>
## Canonical References

### 源项目参考
- `/home/eldwen/sp_vision_25/tasks/auto_aim/detector.hpp` — Detector 接口
- `/home/eldwen/sp_vision_25/tasks/auto_aim/detector.cpp` — Detector 实现
- `/home/eldwen/sp_vision_25/tasks/auto_aim/classifier.hpp` — Classifier 接口
- `/home/eldwen/sp_vision_25/tasks/auto_aim/classifier.cpp` — Classifier 实现

### 目标框架参考
- `tools/tomlpp.hpp` — TOML 读取
- `tools/logger.hpp` — 日志宏定义
- `tools/img_tools.hpp` — 绘制工具（已迁移）
- `app/auto_aim/armor.hpp` — 装甲板数据结构（已迁移）
</canonical_refs>

<code_context>
## Existing Code Insights

### 工具依赖
- `tools::draw_text()` / `tools::draw_points()` — Phase 2 已迁移
- `tools::logger()` → `LOG_XXX` — 需做文本替换
- `fmt::format()` — 需保留，加 fmt 依赖
- `toml::parse_file()` — 替代 `YAML::LoadFile()`

### 注意
- OpenVINO 需要 apt 安装（sudo apt install openvino）
- 配置文件示例需要在 Phase 7 创建

</code_context>

<specifics>
## Specific Ideas

- YAML → TOML 时保持参数名一致，方便对照
- 配置阈值参数保留原始数值

</specifics>

<deferred>
## Deferred Ideas

None

</deferred>

---
*Phase: 3-detector*
*Context gathered: 2026-05-01*
