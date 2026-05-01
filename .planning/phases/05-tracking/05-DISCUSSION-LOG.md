# Phase 5: EKF 目标跟踪 - Discussion Log

**Date:** 2026-05-02
**Areas discussed:** 全向感知, EKF位置, Target特判, Tracker配置, 图像中心

---

## 全向感知依赖
- 删掉 Track 的第二个重载
- 以后开哨兵独立分支

## EKF 位置
- 放 tools/，通用算法

## Target 特判
- outpost/base 注释掉
- balance 保留

## 图像中心硬编码
- 改为 center_norm 归一化坐标
- 发现原作者自带了 TODO 注释

## Voter
- 直接迁移
