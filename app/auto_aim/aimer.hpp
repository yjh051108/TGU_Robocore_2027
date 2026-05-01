#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_AIMER_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_AIMER_HPP
#pragma once

#include <Eigen/Dense>
#include <chrono>
#include <list>

#include "app/auto_aim/command.hpp"
#include "app/auto_aim/target.hpp"

namespace app::auto_aim {

struct AimPoint {
  bool valid;
  Eigen::Vector4d xyza;
};

class Aimer {
public:
  AimPoint debug_aim_point;
  explicit Aimer(const std::string & config_path);
  Command aim(
    std::list<Target> targets, std::chrono::steady_clock::time_point timestamp, double bullet_speed,
    bool to_now = true);

  Command aim(
    std::list<Target> targets, std::chrono::steady_clock::time_point timestamp, double bullet_speed,
    ShootMode shoot_mode, bool to_now = true);

private:
  double yaw_offset_;
  double pitch_offset_;
  double comming_angle_;
  double leaving_angle_;
  double lock_id_ = -1;
  double high_speed_delay_time_;
  double low_speed_delay_time_;
  double decision_speed_;
  std::optional<double> left_yaw_offset_, right_yaw_offset_;

  AimPoint choose_aim_point(const Target & target);
};

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_AIMER_HPP
