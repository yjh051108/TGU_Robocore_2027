# Phase 4: PnP 解算 - Discussion Log

> **Audit trail only.** Decisions are captured in CONTEXT.md.

**Date:** 2026-05-01
**Phase:** 4-PnP 解算
**Areas discussed:** 配置格式, 前哨站逻辑, 重投影代价, yaw参数

---

## 配置格式

**Decision:** YAML→TOML，矩阵存扁平数组
**Notes:** 解释了 toml++ 读法，索引和解算不受影响

---

## 前哨站特殊逻辑 (oupost_reprojection_error)

**Decision:** 去掉
**Reason:** 前哨站装甲板倾斜安装，但是该函数可省略

---

## 重投影代价函数

| Option | Description |
|--------|-------------|
| 简单欧氏距离 | 4 角点像素差之和，简单稳定，原项目当前在用 |
| SJTU_cost | 考虑长度差+角度差，已放弃 |

**Decision:** 简单欧氏距离
**Notes:** 通过 git log 找到 commit e143e15 "give up SJTU_cost function"。分析了原因：SJTU 用猜测的倾斜角加权，和欧氏距离效果差不多，被放弃

---

## yaw 优化参数

**Decision:** SEARCH_RANGE 改为 TOML 可配置
**Notes:** 默认值 140°、步长 1° 保持原样

---

## Claude's Discretion

- 3D 模型点常量保持硬编码
