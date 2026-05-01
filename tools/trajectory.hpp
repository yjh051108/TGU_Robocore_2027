#ifndef TGU_ROBOCORE_2027_TOOLS_TRAJECTORY_HPP
#define TGU_ROBOCORE_2027_TOOLS_TRAJECTORY_HPP
#pragma once

namespace tools {

struct Trajectory {
  bool unsolvable;
  double fly_time;
  double pitch;

  Trajectory(const double v0, const double d, const double h);
};

}  // namespace tools

#endif  // TGU_ROBOCORE_2027_TOOLS_TRAJECTORY_HPP
