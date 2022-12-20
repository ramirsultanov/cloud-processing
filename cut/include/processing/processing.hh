#ifndef PROCESSING_HH
#define PROCESSING_HH

#include <array>

#include <opencv4/opencv2/opencv.hpp>

namespace pro {

class Proc {
public:
  constexpr Proc() = default;
  cv::Mat operator()(cv::Mat in, const std::array<cv::Point, 4>& vertices) const;
protected:
private:
};

} /// namespace pro

namespace pro {

cv::Mat Proc::operator()(cv::Mat in, const std::array<cv::Point, 4>& vertices) const {
  cv::Rect myROI(vertices[0], vertices[2]);
  in = in(myROI);
  return in;
}

} /// namespace pro

#endif /// PROCESSING_HH
