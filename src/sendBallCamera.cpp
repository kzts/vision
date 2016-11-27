//etc
#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
//#include <string.h>
#include <string>
#include <sstream>
//opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
// socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// ip
#include <unistd.h> /* for close */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>

using namespace std;
using namespace cv;

#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480
#define CAMERA_FPS 60
#define END_TIME_MS 10000
//#define END_TIME_MS 1000
#define XY 2
#define NUM 99999
#define NUM_BALL_PARAMS 11

// ball
int mov_params[XY+1];
double pos_ctr[XY];
double bal_ctr[XY];
int params_roi[XY+XY];
unsigned int threshold_gray = 50; 
unsigned int len_roi = 30; 
Mat old_img, dst_img;
int view_mode = 0;

int camera_num;

// socket
char* ip_address;
char buffer[1024];

// time
struct timeval ini_t, now_t;

// save 
char   filename_dat[NUM];
char   filename_avi[NUM];
Mat    frame_avi[NUM];        
double data_time[NUM];
double data_ball[NUM][NUM_BALL_PARAMS];
int codec = CV_FOURCC('X', 'V', 'I', 'D');

char camera_num_file[] = "../data/params/camera_number.dat";
char view_mode_file[] = "../data/params/view_mode.dat";

void setBallParameters(int i){
  data_ball[i][ 0] = pos_ctr[0];
  data_ball[i][ 1] = pos_ctr[1];
  data_ball[i][ 2] = bal_ctr[0];
  data_ball[i][ 3] = bal_ctr[1];
  data_ball[i][ 4] = params_roi[0];
  data_ball[i][ 5] = params_roi[1];
  data_ball[i][ 6] = params_roi[2];
  data_ball[i][ 7] = params_roi[3];
  data_ball[i][ 8] = (double) mov_params[0];
  data_ball[i][ 9] = (double) mov_params[1];
  data_ball[i][10] = (double) mov_params[2];
}

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
  
  sprintf( filename_dat, "../data/dat/%04d%02d%02d/%02d_%02d_%02d.dat", year, month, day, hour, minute, second);
  sprintf( filename_avi, "../data/avi/%04d%02d%02d/%02d_%02d_%02d.avi", year, month, day, hour, minute, second);
  //cout << filenameAvi << endl;
}

void saveAvi( int num ){
  VideoWriter writer( filename_avi, 
		      codec, CAMERA_FPS, Size( CAMERA_WIDTH, CAMERA_HEIGHT ), true );
  
  if (!writer.isOpened())
    cout  << "Could not open the output video." << endl;
  else
    for( unsigned int n = 0; n < num; n++ )
      writer << frame_avi[n];
}

void saveDat( int num ){
  ofstream ofs( filename_dat );
  for( unsigned int n = 0; n < num; n++ ){
    ofs << data_time[n] << "\t";    
    for( unsigned int m = 0; m < NUM_BALL_PARAMS; m++ )
      ofs << data_ball[n][m] << "\t";    
    ofs  << endl;
  }
  //for( unsigned int n = 0; n < num; n++ )
    //ofs << data_time[n] << endl;
}

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
    //for ( int i = 0; i++; i < (XY+XY) )
    for ( int i = 0; i < (XY+XY); i++ )
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
  mov_params[0] = x_sum;
  mov_params[1] = y_sum;
  mov_params[2] = n_sum;
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

//int setServer(void){
int setServer( char* ip_address_, int num_port_ ){
  // server
  int welcomeSocket, newSocket_;
  //char buffer[1024];
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;

  welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  //serverAddr.sin_port = htons(7891);
  serverAddr.sin_port = htons(port_num_);
  serverAddr.sin_addr.s_addr = inet_addr(ip_address_);
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

double getElaspedTime( int i ){
  double now_time_;
  gettimeofday( &now_t, NULL );
  now_time_ =
    + 1000.0*( now_t.tv_sec - ini_t.tv_sec )
    + 0.001*( now_t.tv_usec - ini_t.tv_usec );

  data_time[i] = now_time_;
  return now_time_;
}

char* getIP(void){
  int fd;
  struct ifreq ifr;
  
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  ifr.ifr_addr.sa_family = AF_INET; // IPv4 IP address
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);   // eth0 IP address
  ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);

  // printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

  return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

int getCameraNumber(void){
 ifstream ifs( camera_num_file );
 string str;
 int camera_num_;

 ifs >> str;
 //cout << str << endl;
 istringstream is(str);
 is >> camera_num_;
 return camera_num_;
}

int getViewMode(void){
 ifstream ifs( view_mode_file );
 string str;
 int view_mode_;

 ifs >> str;
 //cout << str << endl;
 istringstream is(str);
 is >> view_mode_;
 return view_mode_;
}


int main(int argc, char* argv[])
{
  ip_address = getIP();
  camera_num = getCameraNumber();
  view_mode = getViewMode();
  cout << "ip: " << ip_address 
       << ", camera: " << camera_num
       << ", view mode: " << view_mode << endl;
  /*
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
  */
  int num_port = 7891;
  // open socket
  //int newSocket = setServer();
  int newSocket = setServer( ip_address, num_port );

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
  //struct timeval ini, now;
  //gettimeofday(&ini, NULL);
  gettimeofday( &ini_t, NULL );

  if ( view_mode > 0 )
    namedWindow( "dst", CV_WINDOW_AUTOSIZE);
  cap >> src_img;
  cvtColor( src_img, gry_img, CV_BGR2GRAY );
  //old_img = gry_img.clone();
  GaussianBlur( gry_img, old_img, Size(5,5), 2, 2 ); 

  // loop
  getFileName();
  int i = 0;
  while (true){
    double now_ms = getElaspedTime(i);
    //gettimeofday(&now, NULL);
    
    cap >> src_img;
    //if ( now.tv_sec - ini.tv_sec > END_TIME_SEC )
    if ( now_ms > END_TIME_MS ){
      // close socket  
      strcpy( buffer, "END" );
      send( newSocket, buffer, 1024, 0);
      break;
    }

    getCircleCenter( src_img );
      
    if ( view_mode > 0 )
      imshow( "dst", dst_img );
      
    //sprintf( buffer, "%d %d", (int) pos_ctr[0], (int) pos_ctr[1] );
    sprintf( buffer, "%d %d", (int) bal_ctr[0], (int) bal_ctr[1] );
    send( newSocket, buffer, 13, 0);

    //frame_avi[i] = src_img;
    frame_avi[i] = src_img.clone();
    setBallParameters(i);
    i++;
    
    waitKey(1);
  }

  // save data
  saveDat(i);
  cout << "saved dat file: " << filename_dat << endl;
  saveAvi(i);
  cout << "saved avi file: " << filename_avi << endl;

  return 0;

}



