#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

struct ColorRange
{
    std::string name;
    cv::Scalar low;
    cv::Scalar high;
};

struct DetectedBall
{
    std::string colorName;
    cv::Point2f pixelPosition;
    float radius = 0.f;
};

class BallDetector
{
public:
    BallDetector();

    void setColorRange(const std::string& colorName, cv::Scalar hsvLow, cv::Scalar hsvHigh);
    cv::Mat getColorMask(const cv::Mat& frame, const std::string& colorName) const;

    std::vector<DetectedBall> detectBalls(const cv::Mat& frame, int minRadius = 6, int maxRadius = 30) const;
    std::vector<DetectedBall> detectBallsByShape(const cv::Mat& frame, int minRadius = 6, int maxRadius = 30) const;

private:
    std::vector<ColorRange> m_colorRanges;

    std::vector<DetectedBall> detectForColor(
        const cv::Mat& hsvFrame,
        const ColorRange& range,
        int minRadius,
        int maxRadius
    ) const;

    std::string classifyColorAtPoint(const cv::Vec3b& hsvPixel) const;
};