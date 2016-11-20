//etc
#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <string.h>
//opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
// socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
using namespace cv;
//double PosBall[120*15][2];

#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480
#define CAMERA_FPS 60
//#define AVI_FILE 0
//#define CAMERA 1
#define END_TIME_SEC 10
#define XY 2

double pos_ctr[XY];
double bal_ctr[XY];
int params_roi[XY+XY];
unsigned int threshold_gray = 50; 
unsigned int len_roi = 30; 
Mat old_img, dst_img;
int view_mode = 0;

char* ip_address;
char buffer[1024];

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

    if ( view_mode > 0 ){    
      for( size_t i = 0; i < circles.size(); i++ ){
	Point center( cvRound( circles[i][0] + params_roi[0] ), 
		      cvRound( circles[i][1] + params_roi[1] ));
	int radius =  cvRound(circles[i][2]);
	circle( dst_img, center, 3, Scalar(0,255,0), -1, 8, 0 );
	circle( dst_img, center, radius, Scalar(0,0,255), 3, 8, 0 );
      }
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

int setServer(void){
  // server
  int welcomeSocket, newSocket_;
  //char buffer[1024];
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;

  welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(7891);
  //serverAddr.sin_addr.s_addr = inet_addr("192.168.2.247");
  serverAddr.sin_addr.s_addr = inet_addr(ip_address);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
  bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  if(listen(welcomeSocket,5)==0)
    printf("Listening\n");
  else
    printf("Error\n");

  addr_size = sizeof serverStorage;
  newSocket_ = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);

  return newSocket_;
}


int main(int argc, char* argv[])
{
  // command line input
  if ( argc != 4 ){
    cout << "input four: " 
	 << "view mode (off:0, on:1), " 
	 << "this PC's IP address and " 
	 << "camera number." 
	 << endl;
    return -1;
  }
  view_mode = atoi(argv[1]);
  ip_address = argv[2];
  int camera_num = atoi(argv[3]);

  // open socket
  int newSocket = setServer();

  // open camera  
  VideoCapture cap(camera_num); 
  if(!cap.isOpened()){
    cout << "camera is not found." << endl;
    return -1;
  }else{
    cap.set( CV_CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH );
    cap.set( CV_CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT );
    cap.set( CV_CAP_PROP_FPS, CAMERA_FPS );
  }
  Mat src_img, gry_img;

  // initiation 
  struct timeval ini, now;
  gettimeofday(&ini, NULL);

  if ( view_mode > 0 )
    namedWindow( "dst", CV_WINDOW_AUTOSIZE);
  cap >> src_img;
  cvtColor( src_img, gry_img, CV_BGR2GRAY );
  old_img = gry_img.clone();
  // loop
  while (true){
    gettimeofday(&now, NULL);
    
    cap >> src_img;
    if ( now.tv_sec - ini.tv_sec > END_TIME_SEC )
      break;
      getCircleCenter( src_img );
      
      if ( view_mode > 0 )
	imshow( "dst", dst_img );
      
      //sprintf( buffer, "%d %d", (int) pos_ctr[0], (int) pos_ctr[1] );
      sprintf( buffer, "%d %d", (int) bal_ctr[0], (int) bal_ctr[1] );
      send( newSocket, buffer, 13, 0);
      
      waitKey(1);
  }

  // close socket  
  strcpy( buffer,"END");
  send( newSocket, buffer, 13, 0);

  return 0;
}



