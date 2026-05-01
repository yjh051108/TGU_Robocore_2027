# Phase 3: 灯条检测与装甲板匹配 - Discussion Log

> **Audit trail only.** Decisions are captured in CONTEXT.md.

**Date:** 2026-05-01
**Phase:** 3-灯条检测与装甲板匹配
**Areas discussed:** 配置格式, fmt处理, cv::imshow, OpenVINO, 日志适配, save(), PCA角点, YOLO detect

---

## 配置格式

| Option | Description | Selected |
|--------|-------------|----------|
| 改为 TOML | toml++ 读取，与 Robocore 风格一致 | ✓ |
| 保留 YAML | 加 yaml-cpp 依赖 | |

**User's choice:** 改为 TOML
**Teaching:** 解释了为什么 Robocore 用 TOML（单头文件零依赖、value_or() 安全取值）

---

## fmt 处理

| Option | Description | Selected |
|--------|-------------|----------|
| 改为 std::format | C++20 自带，语法兼容 | |
| 保留 fmt | 加 fmt 库依赖（很小） | ✓ |
| 字符串拼接 | 不用 format | |

**User's choice:** 保留 fmt

---

## cv::imshow

| Option | Description | Selected |
|--------|-------------|----------|
| 保留 + debug 开关 | debug=true 时才显示 | ✓ |
| 注释掉 | 需用时取消注释 | |
| 直接删掉 | 干净但损失调试能力 | |

**User's choice:** 保留 + debug 开关

---

## OpenVINO

| Option | Description | Selected |
|--------|-------------|----------|
| 简化版 | 只保留 get_type() 几何分类 | |
| 保留接口 | 留空函数等后续 | |
| 完整迁移 | 装 OpenVINO，搬 Classifier | ✓ |

**User's choice:** 完整迁移
**Notes:** Claude 之前擅自加上了"不装 OpenVINO"的限制，用户澄清从未做过这个决定。以后所有决策先和用户讨论。

---

## 日志适配

**Decision:** `tools::logger()->debug()` → `LOG_DEBUG("DETECTOR", ...)`

---

## save() 功能

**Decision:** 保留低置信度存图功能

---

## PCA 角点矫正

**Decision:** 不迁移（原项目 commit 583d50a 已禁用，注释"关闭PCA"）
**Teaching:** 通过 git log 找到了作者的提交记录，理解了原因

---

## YOLO detect 重载

**Decision:** 迁移 `detect(Armor&, const cv::Mat&)`

---

## Claude's Discretion

- 算法逻辑和阈值不变
