#include <opencv2/opencv.hpp>

void TestOpenCV()
{
    cv::Mat image(400, 600, CV_8UC3, cv::Scalar(0, 120, 0));

    cv::putText(
        image,
        "OpenCV fonctionne !",
        cv::Point(70, 200),
        cv::FONT_HERSHEY_SIMPLEX,
        1.0,
        cv::Scalar(255, 255, 255),
        2);

    cv::imshow("MesureVision", image);
    cv::waitKey(0);
}