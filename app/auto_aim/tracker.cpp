#include "app/auto_aim/tracker.hpp"

#include "tools/logger.hpp"
#include "tools/math_tools.hpp"
#include "tools/tomlpp.hpp"

static constexpr const char* MODULE = "TRACKER";

namespace app::auto_aim {

Tracker::Tracker(const std::string & config_path, Solver & solver)
: solver_{solver},
  detect_count_(0),
  temp_lost_count_(0),
  state_("lost"),
  pre_state_("lost"),
  last_timestamp_(std::chrono::steady_clock::now())
{
  auto config = toml::parse_file(config_path);
  enemy_color_ = (config["tracker"]["enemy_color"].value_or<std::string>("") == "red") ? Color::red : Color::blue;
  min_detect_count_ = static_cast<int>(config["tracker"]["min_detect_count"].value_or<int64_t>(5));
  max_temp_lost_count_ = static_cast<int>(config["tracker"]["max_temp_lost_count"].value_or<int64_t>(50));
  // outpost_max_temp_lost_count_ = config["tracker"]["outpost_max_temp_lost_count"].value_or<int64_t>(200);  // TODO: sentry branch
  // normal_temp_lost_count_ = max_temp_lost_count_;
}

std::string Tracker::state() const { return state_; }

std::list<Target> Tracker::track(
  std::list<Armor> & armors, std::chrono::steady_clock::time_point t, bool use_enemy_color)
{
  auto dt = tools::delta_time(t, last_timestamp_);
  last_timestamp_ = t;

  if (state_ != "lost" && dt > 0.1) {
    LOG_WARN(MODULE, "[Tracker] Large dt: {:.3f}s", dt);
    state_ = "lost";
  }

  if (use_enemy_color) {
    armors.remove_if([&](const auto_aim::Armor & a) { return a.color != enemy_color_; });
  }

  armors.sort([](const Armor & a, const Armor & b) {
    cv::Point2f img_center(0.5f, 0.5f);
    auto distance_1 = cv::norm(a.center_norm - img_center);
    auto distance_2 = cv::norm(b.center_norm - img_center);
    return distance_1 < distance_2;
  });

  armors.sort(
    [](const auto_aim::Armor & a, const auto_aim::Armor & b) { return a.priority < b.priority; });

  bool found;
  if (state_ == "lost") {
    found = set_target(armors, t);
  } else {
    found = update_target(armors, t);
  }

  state_machine(found);

  if (state_ != "lost" && target_.diverged()) {
    LOG_DEBUG(MODULE, "[Tracker] Target diverged!");
    state_ = "lost";
    return {};
  }

  if (
    std::accumulate(
      target_.ekf().recent_nis_failures.begin(), target_.ekf().recent_nis_failures.end(), 0) >=
    (0.4 * target_.ekf().window_size)) {
    LOG_DEBUG(MODULE, "[Target] Bad Converge Found!");
    state_ = "lost";
    return {};
  }

  if (state_ == "lost") return {};

  std::list<Target> targets = {target_};
  return targets;
}

void Tracker::state_machine(bool found)
{
  if (state_ == "lost") {
    if (!found) return;

    state_ = "detecting";
    detect_count_ = 1;
  }

  else if (state_ == "detecting") {
    if (found) {
      detect_count_++;
      if (detect_count_ >= min_detect_count_) state_ = "tracking";
    } else {
      detect_count_ = 0;
      state_ = "lost";
    }
  }

  else if (state_ == "tracking") {
    if (found) return;

    temp_lost_count_ = 1;
    state_ = "temp_lost";
  }

  else if (state_ == "temp_lost") {
    if (found) {
      state_ = "tracking";
    } else {
      temp_lost_count_++;
      // TODO sentry branch: outpost uses larger temp_lost_count
      // if (target_.name == ArmorName::outpost)
      //   max_temp_lost_count_ = outpost_max_temp_lost_count_;
      // else
      //   max_temp_lost_count_ = normal_temp_lost_count_;

      if (temp_lost_count_ > max_temp_lost_count_) state_ = "lost";
    }
  }
}

bool Tracker::set_target(std::list<Armor> & armors, std::chrono::steady_clock::time_point t)
{
  if (armors.empty()) return false;

  auto & armor = armors.front();
  solver_.solve(armor);

  auto is_balance = (armor.type == ArmorType::big) &&
                    (armor.name == ArmorName::three || armor.name == ArmorName::four ||
                     armor.name == ArmorName::five);

  if (is_balance) {
    Eigen::VectorXd P0_dig{{1, 64, 1, 64, 1, 64, 0.4, 100, 1, 1, 1}};
    target_ = Target(armor, t, 0.2, 2, P0_dig);
  }

  // TODO sentry branch: outpost/base use different armor counts
  // else if (armor.name == ArmorName::outpost) {
  //   Eigen::VectorXd P0_dig{{1, 64, 1, 64, 1, 81, 0.4, 100, 1e-4, 0, 0}};
  //   target_ = Target(armor, t, 0.2765, 3, P0_dig);
  // }
  // else if (armor.name == ArmorName::base) {
  //   Eigen::VectorXd P0_dig{{1, 64, 1, 64, 1, 64, 0.4, 100, 1e-4, 0, 0}};
  //   target_ = Target(armor, t, 0.3205, 3, P0_dig);
  // }

  else {
    Eigen::VectorXd P0_dig{{1, 64, 1, 64, 1, 64, 0.4, 100, 1, 1, 1}};
    target_ = Target(armor, t, 0.2, 4, P0_dig);
  }

  return true;
}

bool Tracker::update_target(std::list<Armor> & armors, std::chrono::steady_clock::time_point t)
{
  target_.predict(t);

  int found_count = 0;
  double min_x = 1e10;
  for (const auto & armor : armors) {
    if (armor.name != target_.name || armor.type != target_.armor_type) continue;
    found_count++;
    min_x = armor.center.x < min_x ? armor.center.x : min_x;
  }

  if (found_count == 0) return false;

  for (auto & armor : armors) {
    if (
      armor.name != target_.name || armor.type != target_.armor_type
      //  || armor.center.x != min_x
    )
      continue;

    solver_.solve(armor);

    target_.update(armor);
  }

  return true;
}

}  // namespace app::auto_aim
