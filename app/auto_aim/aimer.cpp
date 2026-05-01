#include "app/auto_aim/aimer.hpp"

#include "tools/logger.hpp"
#include "tools/math_tools.hpp"
#include "tools/tomlpp.hpp"
#include "tools/trajectory.hpp"

#include <cmath>
#include <vector>

static constexpr const char* MODULE = "AIMER";

namespace app::auto_aim {

Aimer::Aimer(const std::string & config_path)
: left_yaw_offset_(std::nullopt), right_yaw_offset_(std::nullopt)
{
  auto config = toml::parse_file(config_path);
  yaw_offset_ = config["aimer"]["yaw_offset"].value_or<double>(0.0) / 57.3;
  pitch_offset_ = config["aimer"]["pitch_offset"].value_or<double>(0.0) / 57.3;
  comming_angle_ = config["aimer"]["comming_angle"].value_or<double>(0.0) / 57.3;
  leaving_angle_ = config["aimer"]["leaving_angle"].value_or<double>(0.0) / 57.3;
  high_speed_delay_time_ = config["aimer"]["high_speed_delay_time"].value_or<double>(0.0);
  low_speed_delay_time_ = config["aimer"]["low_speed_delay_time"].value_or<double>(0.0);
  decision_speed_ = config["aimer"]["decision_speed"].value_or<double>(0.0);

  auto left_yaw_node = config["aimer"]["left_yaw_offset"];
  auto right_yaw_node = config["aimer"]["right_yaw_offset"];
  if (left_yaw_node && right_yaw_node) {
    left_yaw_offset_ = left_yaw_node.value_or<double>(0.0) / 57.3;
    right_yaw_offset_ = right_yaw_node.value_or<double>(0.0) / 57.3;
    LOG_INFO(MODULE, "[Aimer] successfully loading shootmode");
  }
}

Command Aimer::aim(
  std::list<Target> targets, std::chrono::steady_clock::time_point timestamp, double bullet_speed,
  bool to_now)
{
  if (targets.empty()) return {false, false, 0, 0};
  auto target = targets.front();

  auto ekf = target.ekf();
  double delay_time =
    target.ekf_x()[7] > decision_speed_ ? high_speed_delay_time_ : low_speed_delay_time_;

  if (bullet_speed < 14) bullet_speed = 23;

  auto future = timestamp;
  if (to_now) {
    double dt;
    dt = tools::delta_time(std::chrono::steady_clock::now(), timestamp) + delay_time;
    future += std::chrono::microseconds(int(dt * 1e6));
    target.predict(future);
  } else {
    auto dt = 0.005 + delay_time;
    future += std::chrono::microseconds(int(dt * 1e6));
    target.predict(future);
  }

  auto aim_point0 = choose_aim_point(target);
  debug_aim_point = aim_point0;
  if (!aim_point0.valid) {
    return {false, false, 0, 0};
  }

  Eigen::Vector3d xyz0 = aim_point0.xyza.head(3);
  auto d0 = std::sqrt(xyz0[0] * xyz0[0] + xyz0[1] * xyz0[1]);
  tools::Trajectory trajectory0(bullet_speed, d0, xyz0[2]);
  if (trajectory0.unsolvable) {
    LOG_DEBUG(MODULE, "[Aimer] Unsolvable trajectory0: {:.2f} {:.2f} {:.2f}", bullet_speed, d0, xyz0[2]);
    debug_aim_point.valid = false;
    return {false, false, 0, 0};
  }

  bool converged = false;
  double prev_fly_time = trajectory0.fly_time;
  tools::Trajectory current_traj = trajectory0;
  std::vector<Target> iteration_target(10, target);

  for (int iter = 0; iter < 10; ++iter) {
    auto predict_time = future + std::chrono::microseconds(static_cast<int>(prev_fly_time * 1e6));
    iteration_target[iter].predict(predict_time);

    auto aim_point = choose_aim_point(iteration_target[iter]);
    debug_aim_point = aim_point;
    if (!aim_point.valid) {
      return {false, false, 0, 0};
    }

    Eigen::Vector3d xyz = aim_point.xyza.head(3);
    double d = std::sqrt(xyz.x() * xyz.x() + xyz.y() * xyz.y());
    current_traj = tools::Trajectory(bullet_speed, d, xyz.z());

    if (current_traj.unsolvable) {
      LOG_DEBUG(MODULE, "[Aimer] Unsolvable trajectory in iter {}: speed={:.2f}, d={:.2f}, z={:.2f}", iter + 1,
                bullet_speed, d, xyz.z());
      debug_aim_point.valid = false;
      return {false, false, 0, 0};
    }

    if (std::abs(current_traj.fly_time - prev_fly_time) < 0.001) {
      converged = true;
      break;
    }
    prev_fly_time = current_traj.fly_time;
  }

  Eigen::Vector3d final_xyz = debug_aim_point.xyza.head(3);
  double yaw = std::atan2(final_xyz.y(), final_xyz.x()) + yaw_offset_;
  double pitch = -(current_traj.pitch + pitch_offset_);
  return {true, false, yaw, pitch};
}

Command Aimer::aim(
  std::list<Target> targets, std::chrono::steady_clock::time_point timestamp, double bullet_speed,
  ShootMode shoot_mode, bool to_now)
{
  double yaw_offset;
  if (shoot_mode == ShootMode::left_shoot && left_yaw_offset_.has_value()) {
    yaw_offset = left_yaw_offset_.value();
  } else if (shoot_mode == ShootMode::right_shoot && right_yaw_offset_.has_value()) {
    yaw_offset = right_yaw_offset_.value();
  } else {
    yaw_offset = yaw_offset_;
  }

  auto command = aim(targets, timestamp, bullet_speed, to_now);
  command.yaw = command.yaw - yaw_offset_ + yaw_offset;

  return command;
}

AimPoint Aimer::choose_aim_point(const Target & target)
{
  Eigen::VectorXd ekf_x = target.ekf_x();
  std::vector<Eigen::Vector4d> armor_xyza_list = target.armor_xyza_list();
  auto armor_num = armor_xyza_list.size();

  if (!target.jumped) return {true, armor_xyza_list[0]};

  auto center_yaw = std::atan2(ekf_x[2], ekf_x[0]);

  std::vector<double> delta_angle_list;
  for (size_t i = 0; i < armor_num; i++) {
    auto delta_angle = tools::limit_rad(armor_xyza_list[i][3] - center_yaw);
    delta_angle_list.emplace_back(delta_angle);
  }

  if (std::abs(target.ekf_x()[8]) <= 2 && target.name != ArmorName::outpost) {
    std::vector<int> id_list;
    for (size_t i = 0; i < armor_num; i++) {
      if (std::abs(delta_angle_list[i]) > 60 / 57.3) continue;
      id_list.push_back(static_cast<int>(i));
    }

    if (id_list.empty()) {
      LOG_WARN(MODULE, "Empty id list!");
      return {false, armor_xyza_list[0]};
    }

    if (id_list.size() > 1) {
      int id0 = id_list[0], id1 = id_list[1];
      if (lock_id_ != id0 && lock_id_ != id1)
        lock_id_ = (std::abs(delta_angle_list[id0]) < std::abs(delta_angle_list[id1])) ? id0 : id1;

      return {true, armor_xyza_list[static_cast<size_t>(lock_id_)]};
    }

    lock_id_ = -1;
    return {true, armor_xyza_list[static_cast<size_t>(id_list[0])]};
  }

  double coming_angle = comming_angle_;
  double leaving_angle = leaving_angle_;

  for (size_t i = 0; i < armor_num; i++) {
    if (std::abs(delta_angle_list[i]) > coming_angle) continue;
    if (ekf_x[7] > 0 && delta_angle_list[i] < leaving_angle) return {true, armor_xyza_list[i]};
    if (ekf_x[7] < 0 && delta_angle_list[i] > -leaving_angle) return {true, armor_xyza_list[i]};
  }

  return {false, armor_xyza_list[0]};
}

}  // namespace app::auto_aim
