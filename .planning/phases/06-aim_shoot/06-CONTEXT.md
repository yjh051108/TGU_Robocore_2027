# Phase 6: 弹道预测与瞄准 - Context

**Gathered:** 2026-05-02
**Status:** Ready for planning

<domain>
## Phase Boundary

迁移弹道预测（Trajectory）、瞄准点选择（Aimer）和射击判定（Shooter）模块。

</domain>

<decisions>
## Implementation Decisions

### Command/ShootMode 类型定义
- **D-01:** 在 `app/auto_aim/command.hpp` 中定义 Command 和 ShootMode
- **D-02:** namespace `app::auto_aim`
- **D-03:** 去掉 `horizon_distance` 字段（UAV 专有，不需要）

### Trajectory
- **D-04:** 放 `tools/trajectory.hpp/.cpp`（纯数学弹道模型）
- **D-05:** namespace `tools`，无需改动

### Aimer
- **D-06:** 放 `app/auto_aim/aimer.hpp/.cpp`
- **D-07:** namespace `app::auto_aim`
- **D-08:** YAML→TOML 配置
- **D-09:** `tools::logger()` → `LOG_XXX`

### Shooter
- **D-10:** 放 `app/auto_aim/shooter.hpp/.cpp`
- **D-11:** namespace `app::auto_aim`
- **D-12:** YAML→TOML 配置

</decisions>

<canonical_refs>
## Canonical References

### 源项目
- `/home/eldwen/sp_vision_25/tools/trajectory.hpp/.cpp`
- `/home/eldwen/sp_vision_25/tasks/auto_aim/aimer.hpp/.cpp`
- `/home/eldwen/sp_vision_25/tasks/auto_aim/shooter.hpp/.cpp`
- `/home/eldwen/sp_vision_25/io/command.hpp` (Command 结构体)
- `/home/eldwen/sp_vision_25/io/cboard.hpp` (ShootMode 枚举)

</canonical_refs>

<deferred>
## Deferred Ideas

None

</deferred>

---
*Phase: 6-aim_shoot*
*Context gathered: 2026-05-02*
