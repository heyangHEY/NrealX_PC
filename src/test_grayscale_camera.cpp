// Reference: https://community.nreal.ai/t/ov-580-in-windows/561/9

#include <opencv2/opencv.hpp>
#include <chrono>

using namespace std;
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
const int FRAME_FPS = 30;

void ConvertYUV222To2xGRAY(cv::Mat &yuvImg, cv::Mat &grayImg);

int main(int argc, char *argv[])
{
    cv::VideoCapture camera = cv::VideoCapture(); // TODO：指定VideoCaptureAPIs参数
    camera.open(0); // TODO: 改成根据vip和pid来获取摄像头的接口号
    if(!camera.isOpened())
    {
        cout << "无法得到视频数据！" << endl;
        return -1;
    }
    // 从 nreal x/light 上拿到的灰度图，帧结构分析：
    // frame 大小为 640*481*2。481中的1是拼接上来的时间戳，去掉它。图像部分就是640*480*2。每个像素占两个字节，直接用amcap或potplayer（YUV422）看，是一张左右双目的拼接图片，宽度方向都被压缩到一半，单色图片（看起来是绿色而不是灰色）。
    // 这实际是由两个 640*480 的灰度图（左右鱼眼各一个）拼接得到的。
    // 所以要获取左右鱼眼灰度图，转换过程：left+right: 640*480*2 -> left: 320*480*2, right：320*480*2 -> left: 640*480*1, right: 640*480*1
        // 获取的frame大小为640*481*2（每个像素占两个字节），所以单目是320*481*2，我们要恢复成640*481*1（每个像素占一个字节）
        // 输入为单个像素占据两个字节（两个通道），所以软件误以为是YUV422格式，但实际上两个通道存储的是两个像素的灰度
        // 只要将【一个像素两个通道】展开成【两个像素，每个像素一个通道】即可
    // frame的数据结构是自定义的，所以不能依赖opencv的自动转换，另外拿到的图片看起来是去畸变的。
    
    camera.set(cv::CAP_PROP_CONVERT_RGB, 0); // 禁止自动转换成rgb图像
    camera.set(cv::CAP_PROP_MODE, cv::CAP_MODE_YUYV); // 以YUYV的方式存储在Mat里，这样就是一个像素两个通道了
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480 + 1); // 最后一行是时间戳数据
    camera.set(cv::CAP_PROP_FPS, FRAME_FPS);

    cv::namedWindow("OV580", cv::WINDOW_NORMAL);
    cvResizeWindow("OV580", 640 * 2, 480); // 最后展开成 1280*480，其中单目是 640*480

    cv::Mat frame;
    cv::Mat grayFrame( 480, 640 * 2, CV_8UC1 ); // 1280*480
    while(true)
    {
        bool success = camera.read(frame); // read() 的效率比 >> 高
        if(!success)
        {
            cout << "无法读取视频帧数据！" << endl;
            break;
        }
        // cout << frame.channels() << endl; // 输出2
        auto t1 = chrono::steady_clock::now();
        ConvertYUV222To2xGRAY(frame, grayFrame);
        auto t2 = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
        cout << duration / 1000.0 << endl;

        // cout << grayFrame.channels() << endl; // 输出1
        cv::imshow("OV580", grayFrame);
        if(cv::waitKey(1) == 27)
        {
            break;
        }
    }

    camera.release();
    cvDestroyAllWindows();
    return 0;
}

// 耗时3.5ms左右
// release+o3编译时，0.5ms左右
// void ConvertYUV222To2xGRAY(cv::Mat &yuyvImg, cv::Mat &grayImg)
// {
//     for(size_t i = 0; i < yuyvImg.rows; ++i)
//     {
//         for(size_t j = 0; j < yuyvImg.cols; ++j)
//         {
//             cv::Vec2b &pixel = yuyvImg.at<cv::Vec2b>(i, j);
//             grayImg.at<uchar>(i, j*2+0) = pixel[0];
//             grayImg.at<uchar>(i, j*2+1) = pixel[1];
//         }
//     }
// }

// 耗时1.5ms左右
// release+o3编译时，0.05ms左右
void ConvertYUV222To2xGRAY(cv::Mat &yuyvImg, cv::Mat &grayImg)
{
    // int rowSize = yuyvImg.rows - 1;
    // int colSize = yuyvImg.cols * yuyvImg.channels();
    for(size_t i = 0; i < 480; ++i)
    {
        uchar* pYuyvImgRow = yuyvImg.ptr<uchar>(i);
        uchar* pGrayImgRow = grayImg.ptr<uchar>(i);
        for(size_t j = 0; j < 640 * 2; ++j)
        {
            pGrayImgRow[j] = pYuyvImgRow[j];
        }
    }
}
