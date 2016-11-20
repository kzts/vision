//#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#define NUM 99999

char   filenameDat[NUM];
char   filenameAvi[NUM];
Mat    frameAvi[NUM];        
double timeDat [NUM];
unsigned int sec, fps, w, h;

int codec = CV_FOURCC('X', 'V', 'I', 'D');

void getFileName(void){
  // set file name
  struct tm *date;
  time_t now;
  
  time(&now);
  date = localtime(&now);

  int year   = date->tm_year + 1900;
  int month  = date->tm_mon + 1;
  int day    = date->tm_mday;
  int hour   = date->tm_hour;
  int minute = date->tm_min;
  int second = date->tm_sec;
  
  sprintf( filenameDat, "../data/dat/%04d%02d%02d/%02d_%02d_%02d.dat", year, month, day, hour, minute, second);
  sprintf( filenameAvi, "../data/avi/%04d%02d%02d/%02d_%02d_%02d.avi", year, month, day, hour, minute, second);
  //cout << filenameAvi << endl;
}

void saveAvi( int Num ){
  VideoWriter writer( filenameAvi, codec, fps, Size( w, h ), true );
  
  if (!writer.isOpened())
    cout  << "Could not open the output video." << endl;
  else
    for( unsigned int n = 0; n < Num; n++ )
      writer << frameAvi[n];
}

void saveDat( int Num ){
  ofstream ofs( filenameDat );
  for( unsigned int n = 0; n < Num; n++ )
    ofs << timeDat[n] << endl;
}

int getVideo(void){
  //VideoCapture cap(0); 
  VideoCapture cap(1); 
  if(!cap.isOpened()){
    cout << "camera is not found." << endl;
    return -1;
  }else{
    cap.set( CV_CAP_PROP_FRAME_WIDTH, w );
    cap.set( CV_CAP_PROP_FRAME_HEIGHT, h );
    cap.set( CV_CAP_PROP_FPS, fps );

    int64 start = getTickCount();
    //int64 old = getTickCount();
    
    //Mat input_image;
    //namedWindow( "window1", 1 );
    
    // loop  
    int n;
    for( n = 0; n < NUM; n++ ){
      //while (1){
      Mat frame;        
      cap >> frame;
      //input_image = frame;
      //imshow("window1", input_image);
      if( waitKey(1) >= 0 ) break;
      
      int64 end = getTickCount();
      //double elapsedMsec = (end - start) * 1000 / getTickFrequency();
      double elapsedSec = (end - start) / getTickFrequency();
      //double diff = (end - old) * 1000 / getTickFrequency();
      //cout << "elasped time: " << elapsedMsec << " ms. \t time interval: " << diff << " ms." << endl;
      
      frameAvi[n] = frame;
      timeDat [n] = (end - start) * 1000 / getTickFrequency();
      
      //old = end;
      if ( elapsedSec > sec )
	break;
    }
    return n;
  }
}

int main( int argc, char*argv[] ){
  // video parameters  
  if ( argc != 5 ){
    cout << "input: sec, fps, width and height." << endl;
    return -1;
  }else{
    sec = atoi( argv[1] );
    fps = atoi( argv[2] );
    w   = atoi( argv[3] );
    h   = atoi( argv[4] );

    getFileName();
    int n = getVideo();
    if ( n > 0 ){
      saveAvi(n);
      saveDat(n);
    }
    return 0;
  }
}




