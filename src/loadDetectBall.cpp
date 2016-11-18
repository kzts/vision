#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>
//#include <string>

using namespace std;
using namespace cv;

//#define NUM_BAK 10
#define NUM_BAK 120
#define NUM_BK2 20
//#define NUM_BK2 2

double PosBall[120*15][2];

int main(int argc, char* argv[]){
  //VideoCapture cap("video.avi");
  VideoCapture cap(argv[1]);
  Mat src_img, gry_img, old_img, dif_img, smt_img, bin_img, dst_img, work_img;
  Mat bak_img, bak_imgs[NUM_BAK];

  /*
  namedWindow( "src", CV_WINDOW_AUTOSIZE);
  namedWindow( "gry", CV_WINDOW_AUTOSIZE);
  namedWindow( "dif", CV_WINDOW_AUTOSIZE);
  namedWindow( "smt", CV_WINDOW_AUTOSIZE);
  */

  namedWindow( "bin", CV_WINDOW_AUTOSIZE);
  namedWindow( "dst", CV_WINDOW_AUTOSIZE);
  namedWindow( "bak", CV_WINDOW_AUTOSIZE);

  unsigned int threshold_gray = 50; 
  unsigned int len_roi = 30; 

  cap >> src_img;
  cvtColor( src_img, gry_img, CV_BGR2GRAY );
  old_img = gry_img.clone();
  bak_img = gry_img.clone();

  for ( int i = 0; i < NUM_BAK; i++ )
    bak_imgs[i] = gry_img;
    //bak_imgs[i] = gry_img.clone();

  for ( int frame = 0; frame < 380; frame++ ){
    cap >> src_img;
    cvtColor( src_img, gry_img, CV_BGR2GRAY );

    for ( int j = 1; j < NUM_BAK; j++ )
      bak_imgs[j-1] = bak_imgs[j].clone();
      //bak_imgs[j] = bak_imgs[j-1];
    //bak_imgs[j] = bak_imgs[j-1].clone();
    //bak_imgs[0] = gry_img.clone();
    //bak_imgs[0] = gry_img;
    bak_imgs[NUM_BAK - 1] = gry_img.clone();
  }

  int num_frame = 0;

  for ( int frame = 0; frame < 100; frame++ ){
    //while (true){
    //load
    cap >> src_img;
    if ( src_img.empty() )
      break;

    // detect circle    
    dst_img = src_img.clone();
    cvtColor( src_img, gry_img, CV_BGR2GRAY ); // convert to gray
    
    for ( int x = 0; x < bak_img.cols; x++ ){
      for ( int y = 0; y < bak_img.rows; y++ ){
	double c = 0;
	//for ( int b = 0; b < NUM_BAK; b++ )
	for ( int b = 0; b < NUM_BK2; b++ )
	  c+= (double) bak_imgs[b].at<unsigned char>(y,x)/ NUM_BK2;
	  //c+= (double) bak_imgs[ NUM_BAK - 1 - b ].at<unsigned char>(y,x)/ NUM_BK2;
	  //c+= (double) bak_imgs[b].at<unsigned char>(y,x)/ NUM_BAK;
	  //c+= bak_imgs[b].at<unsigned char>(y,x);
	//bak_img.at<unsigned char>(y,x) = (unsigned char) (c / NUM_BAK);
	bak_img.at<unsigned char>(y,x) = (unsigned char) c;
      }
    }
    
    //bak_img = bak_imgs[NUM_BAK - 1].clone();
    //bak_img = bak_imgs[0].clone();
    
    //absdiff( gry_img, old_img, dif_img );
    absdiff( gry_img, bak_img, dif_img );
    GaussianBlur( dif_img, smt_img, Size(7,7), 2, 2 ); // smoothing    
    threshold( dif_img, bin_img, threshold_gray, 255, THRESH_BINARY );

    old_img = gry_img.clone();

    int x_sum = 0, y_sum = 0, n_sum = 0;
    for ( int x = 0; x < bin_img.cols; x++ ){
      for ( int y = 0; y < bin_img.rows; y++ ){
	if ( bin_img.at<unsigned char>(y,x) > threshold_gray ){
	  x_sum += x;
	  y_sum += y;
	  n_sum++;
	}
      }
    }

    //cout << x_sum << "\t" << y_sum << "\t" << n_sum << "\t" << (double) x_sum/ n_sum << endl;
    
    if ( n_sum > 50 ){
      double x_ctr = x_sum/ n_sum;
      double y_ctr = y_sum/ n_sum;
      
      Point center;
      if ( n_sum != 0 ){
	Point center( cvRound( x_ctr ), cvRound( y_ctr ));
	circle( dst_img, center, 10, Scalar(255,0,0), 3, 8, 0 );
      }

      vector<Vec3f> circles;
      Mat smt;

      int x_ini = x_ctr - len_roi;
      int y_ini = y_ctr - len_roi;
      int w_roi = 2* len_roi;
      int h_roi = 2* len_roi;

      if ( x_ini < 0 )
	x_ini = 0;
      if ( y_ini < 0 )
	y_ini = 0;
      if ( x_ini + w_roi > src_img.cols )
	w_roi = src_img.cols - x_ini;
      if ( y_ini + h_roi > src_img.rows )
	h_roi = src_img.rows - y_ini;

      //GaussianBlur( gry_img, smt, Size(7,7), 2, 2 ); // smooth
      GaussianBlur( gry_img, smt, Size(5,5), 2, 2 ); // smooth
      
      //cout << x_ini << "\t" << y_ini << "\t" << w_roi << "\t" << x_ini + w_roi << "\t" << h_roi << "\t" << y_ini + h_roi << endl;

      Mat roi = smt( Rect( x_ini, y_ini, w_roi, h_roi ));
      //Mat roi = gry_img( Rect( x_ini, y_ini, w_roi, h_roi ));
            
      HoughCircles( roi, circles, CV_HOUGH_GRADIENT, 1, 100, 6, 20 ); // detect circle



      
      // draw
      for( size_t i = 0; i < circles.size(); i++ ){
      //for( size_t i = 0; i < 3; i++ ){
	//Point center( cvRound(circles[i][0]), cvRound(circles[i][1]) );
	Point center( cvRound( circles[i][0] + x_ini ), cvRound( circles[i][1] + y_ini ));
	int radius =  cvRound(circles[i][2]);
	circle( dst_img, center, 3, Scalar(0,255,0), -1, 8, 0 );
	circle( dst_img, center, radius, Scalar(0,0,255), 3, 8, 0 );

	if ( i == 0 ){
	  PosBall[ num_frame ][0] = cvRound( circles[i][0]) + x_ini;
	  PosBall[ num_frame ][1] = cvRound( circles[i][1]) + y_ini;
	}
      }
    }

    for ( int j = 1; j < NUM_BAK; j++ )
      bak_imgs[j-1] = bak_imgs[j].clone();
    //bak_imgs[j] = bak_imgs[j-1];
    //bak_imgs[j] = bak_imgs[j-1].clone();
    //bak_imgs[0] = gry_img.clone();
    //bak_imgs[0] = gry_img;
    bak_imgs[NUM_BAK - 1] = gry_img.clone();

    // view
    /*
    imshow( "src", src_img );
    imshow( "gry", gry_img );
    imshow( "dif", dif_img );
    imshow( "smt", smt_img );
    */

    imshow( "bin", bin_img );
    imshow( "dst", dst_img );
    imshow( "bak", bak_img );
    //imshow( "bak", bak_imgs[ NUM_BAK - 1]);
    //imshow( "bak", bak_imgs[0]);
     
    //waitKey(1);
    waitKey(100);
    num_frame++;
  }
  
  ofstream writing_file( "../data/ball.dat" );

  //writing_file << " x \t y " << endl;
  for (int i = 0; i < num_frame; i++)
    writing_file << i << " " << PosBall[i][0] << " " << PosBall[i][1] << endl;
  
  return 0;
}


