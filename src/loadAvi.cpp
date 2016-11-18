#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char* argv[]){
  //VideoCapture cap("video.avi");
  VideoCapture cap(argv[1]);

  namedWindow("image", CV_WINDOW_AUTOSIZE);

  while (true){
      Mat Img;
      cap >> Img;
      if (Img.empty())
	break;
      imshow("image", Img);
      waitKey(16);
  }
  return 0;
}
