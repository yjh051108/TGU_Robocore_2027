#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_CLASSIFIER_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_CLASSIFIER_HPP
#pragma once

#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>
#include <string>

#include "app/auto_aim/armor.hpp"

namespace app::auto_aim {

class Classifier {
public:
  explicit Classifier(const std::string & config_path);

  void classify(Armor & armor);

  void ovclassify(Armor & armor);

private:
  cv::dnn::Net net_;
  ov::Core core_;
  ov::CompiledModel compiled_model_;
};

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_CLASSIFIER_HPP
