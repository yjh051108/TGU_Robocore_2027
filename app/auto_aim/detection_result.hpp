#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_DETECTION_RESULT_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_DETECTION_RESULT_HPP
#pragma once

#include <chrono>
#include <list>

#include "app/auto_aim/armor.hpp"

namespace omniperception {

struct DetectionResult {
  std::list<app::auto_aim::Armor> armors;
  std::chrono::steady_clock::time_point timestamp;
  double delta_yaw;
  double delta_pitch;

  DetectionResult & operator=(const DetectionResult & other) {
    if (this != &other) {
      armors = other.armors;
      timestamp = other.timestamp;
      delta_yaw = other.delta_yaw;
      delta_pitch = other.delta_pitch;
    }
    return *this;
  }
};

}  // namespace omniperception

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_DETECTION_RESULT_HPP
