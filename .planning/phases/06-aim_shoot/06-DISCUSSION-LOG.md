# Phase 6: 弹道预测与瞄准 - Discussion Log

**Date:** 2026-05-02
**Areas discussed:** Command 类型, 弹道模型, 瞄准射击, 配置日志

---

## Command/ShootMode 类型定义
- 放 `app/auto_aim/command.hpp`
- 去掉了 UAV 独有字段

## Trajectory
- 放 `tools/`，namespace tools

## Aimer/Shooter
- 放 `app/auto_aim/`，namespace app::auto_aim
- YAML→TOML + LOG_XXX
