#include "tools/extended_kalman_filter.hpp"

#include <cmath>

namespace tools {

ExtendedKalmanFilter::ExtendedKalmanFilter(
  const Eigen::VectorXd & x0, const Eigen::MatrixXd & P0,
  std::function<Eigen::VectorXd(const Eigen::VectorXd &, const Eigen::VectorXd &)> x_add)
: x(x0), P(P0), x_add(std::move(x_add))
{
  I = Eigen::MatrixXd::Identity(x.size(), x.size());
}

Eigen::VectorXd ExtendedKalmanFilter::predict(const Eigen::MatrixXd & F, const Eigen::MatrixXd & Q)
{
  x = F * x;
  P = F * P * F.transpose() + Q;
  return x;
}

Eigen::VectorXd ExtendedKalmanFilter::predict(
  const Eigen::MatrixXd & F, const Eigen::MatrixXd & Q,
  std::function<Eigen::VectorXd(const Eigen::VectorXd &)> f)
{
  x = f(x);
  P = F * P * F.transpose() + Q;
  return x;
}

Eigen::VectorXd ExtendedKalmanFilter::update(
  const Eigen::VectorXd & z, const Eigen::MatrixXd & H, const Eigen::MatrixXd & R,
  std::function<Eigen::VectorXd(const Eigen::VectorXd &, const Eigen::VectorXd &)> z_subtract)
{
  auto y = z_subtract(z, H * x);
  auto S = H * P * H.transpose() + R;
  auto K = P * H.transpose() * S.inverse();
  x = x_add(x, K * y);
  P = (I - K * H) * P;

  total_count_++;
  last_nis = y.transpose() * S.inverse() * y;
  if (last_nis > 11.34) {
    nis_count_++;
    recent_nis_failures.push_back(1);
  } else {
    recent_nis_failures.push_back(0);
  }
  if (recent_nis_failures.size() > window_size) {
    recent_nis_failures.pop_front();
    nis_count_ = std::count(recent_nis_failures.begin(), recent_nis_failures.end(), 1);
  }

  data["nis"] = last_nis;
  data["nis_rate"] = static_cast<double>(nis_count_) / recent_nis_failures.size();

  return x;
}

Eigen::VectorXd ExtendedKalmanFilter::update(
  const Eigen::VectorXd & z, const Eigen::MatrixXd & H, const Eigen::MatrixXd & R,
  std::function<Eigen::VectorXd(const Eigen::VectorXd &)> h,
  std::function<Eigen::VectorXd(const Eigen::VectorXd &, const Eigen::VectorXd &)> z_subtract)
{
  auto y = z_subtract(z, h(x));
  auto S = H * P * H.transpose() + R;
  auto K = P * H.transpose() * S.inverse();
  x = x_add(x, K * y);
  P = (I - K * H) * P;

  total_count_++;
  last_nis = y.transpose() * S.inverse() * y;
  if (last_nis > 11.34) {
    nis_count_++;
    recent_nis_failures.push_back(1);
  } else {
    recent_nis_failures.push_back(0);
  }
  if (recent_nis_failures.size() > window_size) {
    recent_nis_failures.pop_front();
    nis_count_ = std::count(recent_nis_failures.begin(), recent_nis_failures.end(), 1);
  }

  data["nis"] = last_nis;
  data["nis_rate"] = static_cast<double>(nis_count_) / recent_nis_failures.size();

  return x;
}

}  // namespace tools
