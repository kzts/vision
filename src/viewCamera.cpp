//#include "stdafx.h"
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main( int argc, char*argv[] ){
  VideoCapture cap(0); 
  //VideoCapture cap(1); 
  if(!cap.isOpened()){
    cout << "camera is not found." << endl;
    return -1;
  }
  // video parameters  
  unsigned int sec, fps, w, h;
  if ( argc != 5 ){
    cout << "input: sec, fps, width and height." << endl;
    return -1;
  }else{
    sec = atoi( argv[1] );
    fps = atoi( argv[2] );
    w   = atoi( argv[3] );
    h   = atoi( argv[4] );

    cap.set( CV_CAP_PROP_FRAME_WIDTH, w );
    cap.set( CV_CAP_PROP_FRAME_HEIGHT, h );
    cap.set( CV_CAP_PROP_FPS, fps );
  }

  int64 start = getTickCount();
  int64 old = getTickCount();

  Mat input_image;
  namedWindow( "window1", 1 );

  // loop  
  //for(;;){
  while (1){
    Mat frame;        
    cap >> frame;
    input_image = frame;
    imshow("window1", input_image);
    if( waitKey(1) >= 0 ) break;

    int64 end = getTickCount();
    double elapsedMsec = (end - start) * 1000 / getTickFrequency();
    double elapsedSec = (end - start) / getTickFrequency();
    double diff = (end - old) * 1000 / getTickFrequency();
    cout << "elasped time: " << elapsedMsec << " ms. time interval: " << diff << " ms." << endl;

    old = end;
    if ( elapsedSec > sec )
      break;
  }
  
  return 0;
}
