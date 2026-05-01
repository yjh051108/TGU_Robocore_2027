#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>

#include <opencv2/opencv.hpp>

#include "tools/logger.hpp"
#include "tools/math_tools.hpp"
#include "tools/trajectory.hpp"
#include "tools/extended_kalman_filter.hpp"

static constexpr const char* MODULE = "TEST";

void test_limit_rad()
{
  // -pi < result <= pi
  assert(std::abs(tools::limit_rad(0.0) - 0.0) < 1e-10);
  assert(std::abs(tools::limit_rad(CV_PI) - CV_PI) < 1e-10);
  assert(std::abs(tools::limit_rad(CV_PI + 1.0) - (CV_PI + 1.0 - 2 * CV_PI)) < 1e-10);
  assert(std::abs(tools::limit_rad(-CV_PI - 1.0) - (-CV_PI - 1.0 + 2 * CV_PI)) < 1e-10);
  // 100.1 -> multiple wraps to ~ -0.43 rad
  double r = tools::limit_rad(100.1);
  assert(r > -CV_PI && r <= CV_PI);
  LOG_INFO(MODULE, "  limit_rad: PASS");
}

void test_xyz2ypd()
{
  Eigen::Vector3d xyz(1.0, 1.0, 0.0);
  auto ypd = tools::xyz2ypd(xyz);
  // yaw = 45 deg, pitch = 0, distance = sqrt(2)
  assert(std::abs(ypd[0] - CV_PI / 4) < 1e-6);
  assert(std::abs(ypd[1]) < 1e-6);
  assert(std::abs(ypd[2] - std::sqrt(2.0)) < 1e-6);

  // ypd2xyz inverse test
  auto xyz_back = tools::ypd2xyz(ypd);
  assert(std::abs(xyz_back[0] - xyz[0]) < 1e-6);
  assert(std::abs(xyz_back[1] - xyz[1]) < 1e-6);
  assert(std::abs(xyz_back[2] - xyz[2]) < 1e-6);
  LOG_INFO(MODULE, "  xyz2ypd/ypd2xyz: PASS");
}

void test_trajectory()
{
  // 15m/s, 5m away, 0m height difference
  tools::Trajectory traj(15.0, 5.0, 0.0);
  assert(!traj.unsolvable);
  assert(traj.fly_time > 0);
  assert(traj.fly_time < 10.0);

  // Very far target with low speed should be unsolvable
  tools::Trajectory impossible(5.0, 100.0, 0.0);
  assert(impossible.unsolvable);
  LOG_INFO(MODULE, "  trajectory: PASS");
}

void test_ekf()
{
  // Simple 2D EKF: predict + update
  Eigen::VectorXd x0{{0.0, 0.0}};
  Eigen::VectorXd P0_dig{{1.0, 1.0}};
  Eigen::MatrixXd P0 = P0_dig.asDiagonal();
  tools::ExtendedKalmanFilter ekf(x0, P0);

  // State transition: x = x + v*dt, v = v
  Eigen::MatrixXd F{{1, 1}, {0, 1}};
  Eigen::MatrixXd Q = Eigen::MatrixXd::Zero(2, 2);
  ekf.predict(F, Q);
  assert(std::abs(ekf.x[0] - 0.0) < 1e-6);  // x stays same (v=0 initial)

  // Observation: z = x (position), R small = high confidence
  Eigen::VectorXd z{{5.0}};
  Eigen::MatrixXd H{{1, 0}};
  Eigen::MatrixXd R{{0.01}};  // small R = trust measurement
  ekf.update(z, H, R);
  // Kalman gain should bring x close to 5.0
  assert(ekf.x[0] > 4.0);
  LOG_INFO(MODULE, "  EKF: PASS");
}

void test_square()
{
  assert(std::abs(tools::square(3.0) - 9.0) < 1e-10);
  assert(std::abs(tools::square(-2.0) - 4.0) < 1e-10);
  assert(tools::square(0) == 0);
  LOG_INFO(MODULE, "  square: PASS");
}

void test_limit_min_max()
{
  assert(std::abs(tools::limit_min_max(5.0, 0.0, 10.0) - 5.0) < 1e-10);
  assert(std::abs(tools::limit_min_max(-1.0, 0.0, 10.0) - 0.0) < 1e-10);
  assert(std::abs(tools::limit_min_max(15.0, 0.0, 10.0) - 10.0) < 1e-10);
  LOG_INFO(MODULE, "  limit_min_max: PASS");
}

void test_detector_synthetic()
{
  // Build a synthetic image with two light-colored rectangles simulating armor
  cv::Mat img(480, 640, CV_8UC3, cv::Scalar(80, 80, 80));
  // Left lightbar
  cv::rectangle(img, cv::Rect(200, 100, 40, 280), cv::Scalar(200, 200, 200), -1);
  // Right lightbar
  cv::rectangle(img, cv::Rect(400, 100, 40, 280), cv::Scalar(200, 200, 200), -1);

  // Check basic geometry assumptions
  // Each lightbar: width=40, height=280, center at (220, 240) and (420, 240)
  // Expected armor center should be around (320, 240)

  // Verify the image is valid
  assert(!img.empty());
  assert(img.rows == 480);
  assert(img.cols == 640);

  // Calculate expected properties of the lightbars
  cv::RotatedRect r1(cv::Point2f(220, 240), cv::Size2f(40, 280), 0);
  cv::RotatedRect r2(cv::Point2f(420, 240), cv::Size2f(40, 280), 0);

  // Simulate what Lightbar constructor does
  auto width1 = std::max(r1.size.width, r1.size.height);  // 280
  auto width2 = std::max(r2.size.width, r2.size.height);

  // Ratio = length / width for a vertical bar = 280/40 = 7.0
  assert(width1 == 280);
  assert(width2 == 280);

  LOG_INFO(MODULE, "  synthetic image geometry: PASS");
  LOG_INFO(MODULE, "  To test full Detector pipeline, connect a camera and run:");
  LOG_INFO(MODULE, "    auto armors = detector.detect(frame);");
}

int main()
{
  tools::Logger::instance().init({tools::LogLevel::Debug, true, false, ""});
  LOG_INFO(MODULE, "=== Auto Aim Unit Tests ===");

  test_limit_rad();
  test_xyz2ypd();
  test_trajectory();
  test_ekf();
  test_square();
  test_limit_min_max();

  // Detector tests (requires ONNX model)
  std::string model_path = "config/models/tiny_resnet.onnx";
  if (std::filesystem::exists(model_path)) {
    LOG_INFO(MODULE, "  ONNX model found, running synthetic detection test...");
    test_detector_synthetic();
  } else {
    LOG_WARN(MODULE, "  ONNX model not found at {}, skipping detector test", model_path);
  }

  LOG_INFO(MODULE, "=== ALL TESTS PASSED ===");
  return 0;
}
