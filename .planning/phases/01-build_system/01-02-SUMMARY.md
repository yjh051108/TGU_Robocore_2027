# Plan 01-02 Summary: 创建 CMake 配置文件

**Status:** Complete ✓

## What was done
- 创建 tools/CMakeLists.txt（STATIC 库：crc, logger, foxglove_comm）
- 创建 app/CMakeLists.txt（auto_aim 空框架 STATIC 库）
- 更新顶层 CMakeLists.txt（取消 Eigen3 注释、启用 app 子目录、添加 BOOST_ASIO_DISABLE_CONCEPTS）
- 修复 io/CMakeLists.txt（取消 tools 链接注释）
- 修复 task/CMakeLists.txt（read_toml 添加 tools 链接）

## Verification
- cmake configure 成功 ✓
- cmake build 100% 成功 ✓
- libtools.a ✓ | libio.a ✓ | libauto_aim.a ✓
- 可执行文件 read_toml, test_serial, test_logger, test_foxglove, test_camera ✓
- Eigen3 配置已激活 ✓
- app 子目录已启用 ✓
- io→tools PUBLIC 链接已启用 ✓
