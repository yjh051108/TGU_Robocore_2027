#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_COMMAND_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_COMMAND_HPP
#pragma once

namespace app::auto_aim {

enum class ShootMode { left_shoot, right_shoot };

struct Command {
  bool control;
  bool shoot;
  double yaw;
  double pitch;
};

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_COMMAND_HPP
