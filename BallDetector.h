#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// Plage de couleur HSV pour une bille donnée
struct ColorRange
{
    std::string name;
    cv::Scalar low;
    cv::Scalar high;
};

// Résultat d'une détection de bille sur une image
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

    std::vector<DetectedBall> detectForColor(
        const cv::Mat& hsvFrame,
        const ColorRange& range,
        int minRadius,
        int maxRadius
    ) const;

    std::string classifyColorAtPoint(const cv::Vec3b& hsvPixel) const;

    std::vector<DetectedBall> detectBallsByShape(
        const cv::Mat& frame,
        int minRadius,
        int maxRadius
    ) const;

    std::vector<DetectedBall> detectBalls(
        const cv::Mat& frame,
        int minRadius,
        int maxRadius
    ) const;

private:
    std::vector<ColorRange> m_colorRanges;
};
