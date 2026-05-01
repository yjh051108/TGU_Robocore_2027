# Requirements: 自瞄装甲板迁移

**Defined:** 2026-05-01
**Core Value:** 在 Robocore 框架中实现完整的装甲板自瞄流水线

## v1 Requirements

### 构建系统 (Build System)

- [ ] **BLD-01**: 安装开发工具链（cmake, g++, build-essential）
- [ ] **BLD-02**: 安装 Eigen3 依赖（libeigen3-dev）
- [ ] **BLD-03**: 创建 tools/CMakeLists.txt，编译 crc/logger/foxglove_comm 为 STATIC 库
- [ ] **BLD-04**: 修复 io/CMakeLists.txt 中 tools 链接（取消注释）
- [ ] **BLD-05**: 配置 app/CMakeLists.txt 空框架，启用 app 子目录
- [ ] **BLD-06**: 项目完整 CMake 配置通过

### 数据结构与数学工具 (Data & Math Tools)

- [ ] **DATA-01**: 定义 Color, ArmorType, ArmorName, ArmorPriority 枚举
- [ ] **DATA-02**: 定义 Lightbar 结构体（中心点、角点、角度、长宽比等）
- [ ] **DATA-03**: 定义 Armor 结构体（灯条对、类型、Pose、置信度等）
- [ ] **DATA-04**: 支持传统视觉（灯条对构造）和神经网络构造方式
- [ ] **MATH-01**: 实现 `limit_rad()` 弧度限制函数
- [ ] **MATH-02**: 实现 `eulers()` 旋转矩阵/四元数转欧拉角
- [ ] **MATH-03**: 实现 `xyz2ypd()` / `ypd2xyz()` 直角坐标与球坐标转换
- [ ] **MATH-04**: 实现 `delta_time()` 时间差计算
- [ ] **MATH-05**: 实现坐标转换雅可比矩阵
- [ ] **MATH-06**: 实现 `rotation_matrix()` 欧拉角转旋转矩阵
- [ ] **MATH-07**: 实现 `get_abs_angle()` 向量夹角计算
- [ ] **IMG-01**: 迁移 draw_point / draw_points 绘制函数
- [ ] **IMG-02**: 迁移 draw_text 文本绘制函数
- [ ] **IMG-03**: 适配 Robocore 的 cv::Mat 使用方式

### 灯条检测与装甲板匹配 (Detector)

- [ ] **DET-01**: 实现图像预处理（灰度化、二值化）
- [ ] **DET-02**: 实现轮廓提取和灯条拟合（minAreaRect）
- [ ] **DET-03**: 实现灯条几何校验（角度、长宽比、长度）
- [ ] **DET-04**: 实现灯条颜色判定（RB 通道比较）
- [ ] **DET-05**: 实现装甲板匹配（灯条配对）
- [ ] **DET-06**: 实现装甲板几何校验（宽度比、侧比、矩形误差）
- [ ] **DET-07**: 实现装甲板 ROI 提取（get_pattern）
- [ ] **DET-08**: 实现装甲板共用灯条去重逻辑
- [ ] **DET-09**: 实现 Debug 可视化（标注灯条/装甲板信息）
- [ ] **CLS-01**: 实现基于灯条几何特征的装甲板类型判断（小/大装甲）
- [ ] **CLS-02**: 实现基于名字的类型校验（英雄/基地只能是大装甲）

### PnP 解算 (Solver)

- [ ] **SLV-01**: 定义大/小装甲板 3D 模型点
- [ ] **SLV-02**: 实现 solvePnP 姿态解算（solvePnP_IPPE）
- [ ] **SLV-03**: 实现相机→云台→世界坐标系变换
- [ ] **SLV-04**: 实现 yaw 优化（重投影误差最小化）
- [ ] **SLV-05**: 支持云台姿态更新

### EKF 目标跟踪 (EKF Tracking)

- [ ] **EKF-01**: 实现 ExtendedKalmanFilter 类（predict/update）
- [ ] **EKF-02**: 支持自定义状态加法、观测函数、观测减法
- [ ] **EKF-03**: 实现 NIS 卡方检验
- [ ] **TGT-01**: 实现 11 维状态向量 EKF 目标初始化
- [ ] **TGT-02**: 实现状态预测（状态转移矩阵 + 噪声模型）
- [ ] **TGT-03**: 实现装甲板匹配（多装甲板角度匹配）
- [ ] **TGT-04**: 实现观测更新（ypda 四维观测 + 自适应 R 矩阵）
- [ ] **TGT-05**: 实现装甲板位置计算（考虑旋转半径和长短轴）
- [ ] **TRK-01**: 实现状态机（lost → detecting → tracking → temp_lost）
- [ ] **TRK-02**: 实现新目标设置（set_target）
- [ ] **TRK-03**: 实现目标更新（update_target）
- [ ] **TRK-04**: 实现发散检测
- [ ] **TRK-05**: 实现收敛判断
- [ ] **VOT-01**: 实现装甲板类型多帧投票机制

### 弹道预测与瞄准 (Aim & Shoot)

- [ ] **TRAJ-01**: 实现 Trajectory 弹道解算（pitch 迭代求解）
- [ ] **TRAJ-02**: 支持自定义弹速和目标距离
- [ ] **AIM-01**: 实现瞄准点选择（锁定模式/小陀螺模式）
- [ ] **AIM-02**: 实现弹道迭代解算（最多 10 次）
- [ ] **AIM-03**: 实现 yaw/pitch 偏移量配置
- [ ] **AIM-04**: 支持左右射击模式偏移
- [ ] **SHT-01**: 实现射击条件判断（距离、容忍度）
- [ ] **SHT-02**: 支持自动射击开关

### 集成测试 (Integration Test)

- [ ] **CFG-01**: 创建 `config/auto_aim.toml` 配置文件
- [ ] **TST-01**: 创建 `task/test/test_auto_aim.cpp` 测试程序
- [ ] **TST-02**: 完整流水线编译通过

## v2 Requirements

- **YOLO 检测器集成**: 使用 ONNX/OpenVINO 进行神经网络装甲板检测
- **多线程检测**: 迁移 multithread/ 模块
- **Planner 规划器**: 迁移 TinyMPC 路径规划

## Out of Scope

| Feature | Reason |
|---------|--------|
| 能量机关 (auto_buff) | 用户明确排除 |
| 全向感知 (omniperception) | 独立模块，后续迁移 |
| ROS2 集成 | Robocore 框架不使用 ROS2 |
| Foxglove 可视化 | 已有 tools/foxglove_comm 实现 |
| Planner/TinyMPC | 独立路径规划模块 |
| 云台控制 (gimbal) | io 层已有类似功能，后续适配 |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| BLD-01~06 | Phase 1 | Pending |
| DATA-01~04 | Phase 2 | Pending |
| MATH-01~07 | Phase 2 | Pending |
| IMG-01~03 | Phase 2 | Pending |
| DET-01~09 | Phase 3 | Pending |
| CLS-01~02 | Phase 3 | Pending |
| SLV-01~05 | Phase 4 | Pending |
| EKF-01~03 | Phase 5 | Pending |
| TGT-01~05 | Phase 5 | Pending |
| TRK-01~05 | Phase 5 | Pending |
| VOT-01 | Phase 5 | Pending |
| TRAJ-01~02 | Phase 6 | Pending |
| AIM-01~04 | Phase 6 | Pending |
| SHT-01~02 | Phase 6 | Pending |
| CFG-01 | Phase 7 | Pending |
| TST-01~02 | Phase 7 | Pending |

**Coverage:**
- v1 requirements: 49 total
- Mapped to phases: 49
- Unmapped: 0 ✓

---
*Requirements defined: 2026-05-01*
*Last updated: 2026-05-01 after roadmap restructure*
