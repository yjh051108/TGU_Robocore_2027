#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_SHOOTER_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_SHOOTER_HPP
#pragma once

#include <string>

#include "app/auto_aim/aimer.hpp"
#include "app/auto_aim/command.hpp"

namespace app::auto_aim {

class Shooter {
public:
  Shooter(const std::string & config_path);

  bool shoot(
    const Command & command, const Aimer & aimer,
    const std::list<Target> & targets, const Eigen::Vector3d & gimbal_pos);

private:
  Command last_command_;
  double judge_distance_;
  double first_tolerance_;
  double second_tolerance_;
  bool auto_fire_;
};

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_SHOOTER_HPP
