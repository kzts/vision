#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>

// socket
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <arpa/inet.h>

using namespace std;
using namespace cv;
//double PosBall[120*15][2];

#define XY 2

double pos_ctr[XY];
double bal_ctr[XY];
int params_roi[XY+XY];
unsigned int threshold_gray = 50; 
unsigned int len_roi = 30; 
Mat old_img, dst_img;

/*
void saveBall(void){
  ofstream writing_file( "../data/ball.dat" );

  writing_file << " x \t y " << endl;
  
  for (int i = 0; i < num_frame; i++)
    writing_file << i << " " << PosBall[i][0] << " " << PosBall[i][1] << endl;
}
*/

void getRoiParameters( int cols, int rows ){
  if ( pos_ctr[0] > 0 ){
    int x_ini = pos_ctr[0] - len_roi;
    int y_ini = pos_ctr[1] - len_roi;
    int w_roi = 2* len_roi;
    int h_roi = 2* len_roi;

    if ( x_ini < 0 )
      x_ini = 0;
    if ( y_ini < 0 )
      y_ini = 0;
    if ( x_ini + w_roi > cols )
      w_roi = cols - x_ini;
    if ( y_ini + h_roi > rows )
      h_roi = rows - y_ini;
    //cout << x_ini << "\t" << y_ini << "\t" << w_roi << "\t" << x_ini + w_roi << "\t" << h_roi << "\t" << y_ini + h_roi << endl;
    params_roi[0] = x_ini;
    params_roi[1] = y_ini;
    params_roi[2] = w_roi;
    params_roi[3] = h_roi;
  }else{
    for ( int i = 0; i++; i < (XY+XY) )
      params_roi[i] = -1;
  }
}

void getMovingCenter( Mat bin_img_ ){
  int x_sum = 0, y_sum = 0, n_sum = 0;
  
  for ( int x = 0; x < bin_img_.cols; x++ ){
    for ( int y = 0; y < bin_img_.rows; y++ ){
      if ( bin_img_.at<unsigned char>(y,x) > threshold_gray ){
	x_sum += x;
	y_sum += y;
	n_sum++;
      }
    }
  }
  //cout << x_sum << "\t" << y_sum << "\t" << n_sum << "\t" << (double) x_sum/ n_sum << endl;
  if ( n_sum > 0 ){
    pos_ctr[0] = x_sum/ n_sum;
    pos_ctr[1] = y_sum/ n_sum;
  }else{
    pos_ctr[0] = -1;
    pos_ctr[1] = -1;
  }
}

void drawCircle( Mat smt_img_ ){
  bal_ctr[0] = -1;
  bal_ctr[1] = -1;

  if ( params_roi[0] > 0 ){
    Mat roi = smt_img_( Rect( params_roi[0], params_roi[1], params_roi[2], params_roi[3] ));
    
    vector<Vec3f> circles;            
    HoughCircles( roi, circles, CV_HOUGH_GRADIENT, 1, 100, 6, 20 ); // detect circle
    
    for( size_t i = 0; i < circles.size(); i++ ){
      Point center( cvRound( circles[i][0] + params_roi[0] ), cvRound( circles[i][1] + params_roi[1] ));
      int radius =  cvRound(circles[i][2]);
      circle( dst_img, center, 3, Scalar(0,255,0), -1, 8, 0 );
      circle( dst_img, center, radius, Scalar(0,0,255), 3, 8, 0 );
    }
    if ( circles.size() > 0 ){
      bal_ctr[0] = circles[0][0] + params_roi[0];
      bal_ctr[1] = circles[0][1] + params_roi[1];
    }
  }
}

void getCircleCenter( Mat src_img_ ){
  Mat gry_img, dif_img, smt_img, bin_img, work_img;

  dst_img = src_img_.clone();
  cvtColor( src_img_, gry_img, CV_BGR2GRAY ); 
  GaussianBlur( gry_img, smt_img, Size(5,5), 2, 2 ); 
  absdiff( smt_img, old_img, dif_img ); 
  threshold( dif_img, bin_img, threshold_gray, 255, THRESH_BINARY );
  getMovingCenter( bin_img );
  getRoiParameters( gry_img.cols, gry_img.rows );
  drawCircle( smt_img );
  
  old_img = smt_img.clone();    
}


int main(int argc, char* argv[]){
  // opencv
  VideoCapture cap(argv[1]);
  Mat src_img, gry_img;

  namedWindow( "dst", CV_WINDOW_AUTOSIZE);

  cap >> src_img;
  cvtColor( src_img, gry_img, CV_BGR2GRAY );
  old_img = gry_img.clone();

  // server
  int welcomeSocket, newSocket;
  char buffer[1024];
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;

  welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(7891);
  serverAddr.sin_addr.s_addr = inet_addr("192.168.2.247");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
  bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  if(listen(welcomeSocket,5)==0)
    printf("Listening\n");
  else
    printf("Error\n");

  addr_size = sizeof serverStorage;
  newSocket = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);

  //strcpy(buffer,"Hello World\n");
  //send(newSocket,buffer,13,0);

  // loop
  while (true){
    //load
    cap >> src_img;
    if ( src_img.empty() )
      break;
    getCircleCenter( src_img );
    imshow( "dst", dst_img );
    
    //sprintf( buffer, "%d %d", (int) pos_ctr[0], (int) pos_ctr[1] );
    sprintf( buffer, "%d %d", (int) bal_ctr[0], (int) bal_ctr[1] );
    send( newSocket, buffer, 13, 0);
 
    waitKey(1);
    //waitKey(100);
  }
  
  strcpy( buffer,"END");
  send( newSocket, buffer, 13, 0);

  return 0;
}



