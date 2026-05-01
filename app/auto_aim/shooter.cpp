#include "app/auto_aim/shooter.hpp"

#include "tools/logger.hpp"
#include "tools/math_tools.hpp"
#include "tools/tomlpp.hpp"

static constexpr const char* MODULE = "SHOOTER";

namespace app::auto_aim {

Shooter::Shooter(const std::string & config_path) : last_command_{false, false, 0, 0}
{
  auto config = toml::parse_file(config_path);
  first_tolerance_ = config["shooter"]["first_tolerance"].value_or<double>(0.0) / 57.3;
  second_tolerance_ = config["shooter"]["second_tolerance"].value_or<double>(0.0) / 57.3;
  judge_distance_ = config["shooter"]["judge_distance"].value_or<double>(0.0);
  auto_fire_ = config["shooter"]["auto_fire"].value_or<bool>(false);
}

bool Shooter::shoot(
  const Command & command, const Aimer & aimer,
  const std::list<Target> & targets, const Eigen::Vector3d & gimbal_pos)
{
  if (!command.control || targets.empty() || !auto_fire_) return false;

  auto target_x = targets.front().ekf_x()[0];
  auto target_y = targets.front().ekf_x()[2];
  auto tolerance = std::sqrt(tools::square(target_x) + tools::square(target_y)) > judge_distance_
                     ? second_tolerance_
                     : first_tolerance_;

  if (
    std::abs(last_command_.yaw - command.yaw) < tolerance * 2 &&
    std::abs(gimbal_pos[0] - last_command_.yaw) < tolerance &&
    aimer.debug_aim_point.valid) {
    last_command_ = command;
    return true;
  }

  last_command_ = command;
  return false;
}

}  // namespace app::auto_aim
