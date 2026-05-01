# 自瞄装甲板系统迁移 (sp_vision_25 → TGU_Robocore_2027)

## What This Is

将同济大学 SuperPower 战队 sp_vision_25 项目中的自瞄装甲板识别算法迁移至 TGU_Robocore_2027 框架。本次迁移仅包含装甲板自瞄模块（装甲板识别、PnP 解算、目标跟踪、弹道预测），不包括能量机关模块。

## Core Value

在 Robocore 框架中实现完整的装甲板自瞄流水线：从图像输入 → 灯条检测 → 装甲板匹配 → PnP 解算 → EKF 目标跟踪 → 弹道预测 → 射击指令输出，保持原有算法功能完整性。

## Requirements

### Validated

- sp_vision_25 的 `tasks/auto_aim/` 模块已在实际 Robocore 赛场上验证
- Robocore 框架已有 `app/auto_aim/` 目录结构和占位文件
- Robocore 代码规范（`项目结构与代码风格.md`）已明确

### Active

- [ ] **MATH-01**: 迁移 math_tools（limit_rad, xyz2ypd, delta_time, eulers 等数学工具）
- [ ] **EKF-01**: 迁移 ExtendedKalmanFilter 实现
- [ ] **TRAJ-01**: 迁移 Trajectory 弹道预测模型
- [ ] **DATA-01**: 迁移装甲板数据结构和枚举（Lightbar, Armor, Color, ArmorType, ArmorName）
- [ ] **DET-01**: 迁移 Detector 灯条检测和装甲板匹配算法
- [ ] **CLS-01**: 迁移 Classifier 装甲板分类器
- [ ] **SLV-01**: 迁移 Solver PnP 解算和坐标变换
- [ ] **TGT-01**: 迁移 Target EKF 状态预测
- [ ] **TRK-01**: 迁移 Tracker 状态机和目标管理
- [ ] **VOT-01**: 迁移 Voter 多帧投票
- [ ] **AIM-01**: 迁移 Aimer 瞄准点和弹道迭代计算
- [ ] **SHT-01**: 迁移 Shooter 射击判定
- [ ] **CFG-01**: 创建自瞄模块配置文件（TOML）
- [ ] **BLD-01**: 配置 CMakeLists.txt 构建系统
- [ ] **TST-01**: 创建自瞄模块测试程序

### Out of Scope

- **能量机关（auto_buff）** — 用户明确指定不需要
- **全向感知（omniperception）** — 属于独立模块，不在此次迁移范围
- **YOLO 检测器** — 涉及 ONNX/OpenVINO 模型依赖，作为后续扩展
- **ROS2 集成** — Robocore 框架不使用 ROS2
- **多线程命令生成** — `multithread/` 属于哨兵多线程模式，暂不迁移
- **Planner/TinyMPC 规划器** — 独立于装甲板识别的路径规划模块
- **Foxglove 可视化** — Robocore 已通过 tools/foxglove_comm 实现

## Context

### 源项目 (sp_vision_25)
- 语言: C++17, 使用 OpenCV, Eigen3, fmt, spdlog, yaml-cpp, OpenVINO
- 自瞄模块位于 `tasks/auto_aim/`，包含 detector/classifier/solver/target/tracker/aimer/shooter/voter
- 依赖 `tools/math_tools`, `tools/extended_kalman_filter`, `tools/img_tools`, `tools/trajectory`
- 算法流程: 灯条检测 → 装甲板匹配 → 数字识别 → PnP 解算 → EKF 跟踪 → 弹道预测 → 射击

### 目标框架 (TGU_Robocore_2027)
- 语言: C++20, 使用 Boost, OpenCV, Aravis
- 项目结构: `config/` → `app/` → `io/` ↔ `tools/`
- 已有 `app/auto_aim/` 占位文件
- 代码风格规范已明确定义（蛇形命名、4空格缩进、namespace app::auto_aim）

### 迁移策略
- 保持原有算法逻辑完整性
- 适配 Robocore 代码规范（命名、格式、目录结构）
- 使用 Robocore 的 Logger 替代 spdlog/fmt
- 只迁移装甲板部分模块，不使用 OpenVINO（YOLO 检测暂不迁移）

## Constraints

- **代码风格**: 必须遵循 `项目结构与代码风格.md`（蛇形命名、4空格、namespace app::auto_aim）
- **构建系统**: CMake C++20，需与现有框架一致
- **依赖**: 使用框架已有依赖（Boost, OpenCV, Eigen3），不引入额外重量级依赖
- **算法完整性**: 不得因迁移而丢失原有功能和精度
- **命名空间**: 使用 `app::auto_aim`（遵循 Robocore 的 `app` 命名空间规范）

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| 使用 Robocore 的 Logger | 避免引入 spdlog/fmt 依赖，统一日志系统 | — Pending |
| namespace app::auto_aim | 遵循 `项目结构与代码风格.md` 规范 | — Pending |
| 暂不迁移 YOLO | 涉及 ONNX/OpenVINO，增加依赖复杂度，后续扩展 | — Pending |
| 不使用 OpenVINO | Robocore 框架无此依赖，且只做灯条检测迁移 | — Pending |

---
*Last updated: 2026-05-01 after initialization*
