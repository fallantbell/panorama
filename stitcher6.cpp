#include<opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/core.hpp"
#include<iostream>
#include<math.h>

using namespace cv;
using namespace std;

void stitchimg(Mat &src,Mat &warp,int midline);
void stitch(int, void*);
Mat crop(Mat &result);

Mat img[100];
int maxx[100],minx[100],maxy[100],miny[100];

int picturenum=0;


cv::Point2f convert_pt(cv::Point2f point,int w,int h)
{
    //center the point at 0,0
    cv::Point2f pc(point.x-w/2,point.y-h/2);

    //these are your free parameters
    float f = -w/2;
    float r = w;

    float omega = w/2;
    float z0 = f - sqrt(r*r-omega*omega);

    float zc = (2*z0+sqrt(4*z0*z0-4*(pc.x*pc.x/(f*f)+1)*(z0*z0-r*r)))/(2* (pc.x*pc.x/(f*f)+1)); 
    cv::Point2f final_point(pc.x*zc/f,pc.y*zc/f);
    final_point.x += w/2;
    final_point.y += h/2;
    return final_point;
}

Mat cylin(int pictureindex){
    int width=img[pictureindex].cols;
    int height=img[pictureindex].rows;
    Mat tmpimg(img[pictureindex].size(),CV_8U);

	// initial
	maxx[pictureindex]=width/2;
	minx[pictureindex]=width/2;
	maxy[pictureindex]=height/2;
	miny[pictureindex]=height/2;
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            cv::Point2f current_pos(x,y);
            current_pos = convert_pt(current_pos, width, height);

            cv::Point2i top_left((int)current_pos.x,(int)current_pos.y); //top left because of integer rounding

            //make sure the point is actually inside the original image
            if(top_left.x < 0 ||
            top_left.x > width-2 ||
            top_left.y < 0 ||
            top_left.y > height-2)
            {
                continue;
            }

            //bilinear interpolation
            float dx = current_pos.x-top_left.x;
            float dy = current_pos.y-top_left.y;

            float weight_tl = (1.0 - dx) * (1.0 - dy);
            float weight_tr = (dx)       * (1.0 - dy);
            float weight_bl = (1.0 - dx) * (dy);
            float weight_br = (dx)       * (dy);

            uchar value =   weight_tl * img[pictureindex].at<uchar>(top_left) +
			weight_tr * img[pictureindex].at<uchar>(top_left.y,top_left.x+1) +
			weight_bl * img[pictureindex].at<uchar>(top_left.y+1,top_left.x) +
			weight_br * img[pictureindex].at<uchar>(top_left.y+1,top_left.x+1);

			if(value>0){
				maxx[pictureindex]=max(maxx[pictureindex],x);
				minx[pictureindex]=min(minx[pictureindex],x);
				maxy[pictureindex]=max(maxy[pictureindex],y);
				miny[pictureindex]=min(miny[pictureindex],y);
			}

            tmpimg.at<uchar>(y,x) = value;
        }
    }

	return tmpimg;
}

Mat findtranslation(vector<Point2f> r,vector<Point2f> l){

	/*
		Htmp: 
		[1 , 0 , tx]
		[0 , 1 , ty]
	*/
	Mat Htmp(2,3,CV_64F,Scalar(0));
	Htmp.ptr<double>(0)[0]=1;
	Htmp.ptr<double>(1)[1]=1;

	int matchnum=0;
	// choose some match point
	for(int i=0;i<min(100,int(r.size()));i++){
		double tx,ty;
		tx=l[i].x-r[i].x;
		ty=l[i].y-r[i].y;
		int sum=0;
		/*
			1.calculate this match point translation matrix
			2.use this matrix to translate other point
			3.calculate the error
		*/
		for(int j=0;j<r.size();j++){   
			double difx=double(r[j].x)+tx-double(l[j].x);
			double dify=double(r[j].y)+ty-double(l[j].y);
			if(difx<0){
				difx*=(-1);
			}
			if(dify<0){
				dify*=(-1);
			}
			if(difx+dify<3){ // error < threshold 
				sum++;
			}
		}
		// sum bigger mean this translation matrix is better 
		if(sum>matchnum){
			matchnum=sum;
			Htmp.ptr<double>(0)[2]=tx;
			Htmp.ptr<double>(1)[2]=ty;
		}

	}
	return Htmp;
}


void stitch(int, void *)
{
	double start,end;

	start=clock();

	Ptr<ORB> detector = ORB::create(400);
    Ptr<FastFeatureDetector> fast = FastFeatureDetector::create();  

	vector<KeyPoint> keypoints_img[picturenum],keypoint_result;
	Mat descriptor_img[picturenum],descriptor_result;

	FlannBasedMatcher fbmatcher(new flann::LshIndexParams(20, 10, 2));
	vector<DMatch> matches;

	// find keypoint and descriptor
	for(int i=0;i<picturenum;i++){
		resize(img[i],img[i],Size(img[i].cols/3,img[i].rows/3));
		cvtColor(img[i],img[i],CV_BGR2GRAY);
		copyMakeBorder(img[i],img[i],100,100,100,100,BORDER_CONSTANT);

		// cylindrical projection
    	img[i]=cylin(i);
		img[i]=img[i](Range(miny[i],maxy[i]),Range(minx[i],maxx[i]));

		// use fast algorithm to find keypoint
		fast->detect(img[i],keypoints_img[i],Mat());

		// limit keypoint number
		KeyPointsFilter::retainBest(keypoints_img[i],500);
		
		// use orb to find descriptor
		detector->compute(img[i],keypoints_img[i],descriptor_img[i]);

	}

	Mat result=img[0];
	int movex=0,movey=0;

	for(int k=0;k<picturenum-1;k++){

		// use flann to find match
		fbmatcher.match(descriptor_img[k], descriptor_img[k+1], matches);
		double minDist = 1000;

		vector<DMatch> goodmatches;

		for (int i = 0; i < descriptor_img[k].rows; i++)
		{
			double dist = matches[i].distance;
			if (dist < minDist)
			{
				minDist=dist ;
			}
		}
		/*
			dist smaller is better
			choose some good match to find transation matrix
		*/
		for (int i = 0; i < descriptor_img[k].rows; i++)
		{
			double dist = matches[i].distance;
			if (dist < max(5 * minDist, 0.02))
			{
				goodmatches.push_back(matches[i]);
			}
		}

		vector<Point2f> keypointsl,keypointsr;
		for(int i=0;i<goodmatches.size();i++){
			keypointsr.push_back(keypoints_img[k+1][goodmatches[i].trainIdx].pt);
			keypointsl.push_back(keypoints_img[k][goodmatches[i].queryIdx].pt);
		}
		/*
			H =
			[1 , 0 , tx],
			[0 , 1 , ty]

			tx = x direction length that right picture need to move
			ty = y direction length that right picture need to move
		*/
		Mat H=findtranslation(keypointsr,keypointsl);

		/*
			assume that H12 tx = 5
			it mean that image2 is on the right side of image1 , and distance = 5 pixels

			H23 tx = 3
			it mean that image3 is on the right side of image2 , and distance = 3 pixels

			so image3 is on the right side of image1 , and distance = 5 + 3 = 8 pixels
		*/
		H.ptr<double>(0)[2]+=movex;
		H.ptr<double>(1)[2]+=movey;
		movex=H.ptr<double>(0)[2];
		movey=H.ptr<double>(1)[2];

		// if ty < 0 , it will be wrong
		// you can change the code by yourself to fix it
		int mRows=max(result.rows,img[k+1].rows+int(H.ptr<double>(1)[2]));
		int mCols=img[k+1].cols+int(H.ptr<double>(0)[2]);

		// calculate the midline of region that two picture overlapping 
		int midline=(result.cols+int(H.ptr<double>(0)[2]))/2;

		Mat stitchedImage = Mat::zeros(mRows,mCols,CV_8UC1);

		// use affine translation
		warpAffine(img[k+1],stitchedImage,H,Size(mCols,mRows));

		// stitch two picture
		stitchimg(result,stitchedImage,midline);

		// result = new stitching picture
		result=stitchedImage;
	}
	Mat final=crop(result);
	end=clock();
	cout<<end-start<<endl;

	imshow("result",final);
	imwrite("result.jpg",result);

}

void stitchimg(Mat &src,Mat &warp,int midline){
	// stitch two picture in pixels
	for(int i=0;i<src.rows;i++){
		for(int j=0;j<src.cols;j++){
			// both of two picture are black
			if(warp.ptr<uchar>(i)[j]==0 && src.ptr<uchar>(i)[j]==0){
				warp.ptr<uchar>(i)[j]=0;
			}
			else{
				// right picture is black and left picture is not black , choose left picture
				if(warp.ptr<uchar>(i)[j]==0){
					warp.ptr<uchar>(i)[j]=src.ptr<uchar>(i)[j];
				}
				else{
					// blending
					if(j>midline-20 && j<midline+20){
						float ratio=float((j-midline+20))/40;
						warp.ptr<uchar>(i)[j]=warp.ptr<uchar>(i)[j]*ratio+src.ptr<uchar>(i)[j]*(1-ratio);
					}
					// if pixel is at the left side of blending region , choose left picture
					// else choose right picture 
					if(j<=midline-20){
						warp.ptr<uchar>(i)[j]=src.ptr<uchar>(i)[j];
					}
				}

			}
		}
	}
}

Mat crop(Mat &result){
	int top=0,bottom=result.rows;
	for(int i=0;i<result.cols;i++){
		for(int j=0;j<result.rows;j++){
			if(result.ptr<uchar>(j)[i]!=0){
				top=max(top,j);
				break;
			}
		}
	}
	for(int i=0;i<result.cols;i++){
		for(int j=result.rows-1;j>=0;j--){
			if(result.ptr<uchar>(j)[i]!=0){
				bottom=min(bottom,j);
				break;
			}
		}
	}
	return result(Range(top,bottom),Range(0,result.cols));
}

int main(int argc, char** argv)
{
	// change to your picture number
	picturenum=9;

	img[0]=imread("pond/1.jpg");
	img[1]=imread("pond/2.jpg");
	img[2]=imread("pond/3.jpg");
	img[3]=imread("pond/4.jpg");
	img[4]=imread("pond/5.jpg");
	img[5]=imread("pond/6.jpg");
	img[6]=imread("pond/7.jpg");
	img[7]=imread("pond/8.jpg");
	img[8]=imread("pond/9.jpg");

	// img[0]=imread("ocean/1.jpg");
	// img[1]=imread("ocean/2.jpg");
	// img[2]=imread("ocean/3.jpg");
	// img[3]=imread("ocean/4.jpg");
	// img[4]=imread("ocean/5.jpg");
	// img[5]=imread("ocean/6.jpg");
	// img[6]=imread("ocean/7.jpg");
	// img[7]=imread("ocean/8.jpg");
	// img[8]=imread("ocean/9.jpg");
	// img[9]=imread("ocean/10.jpg");
	
	stitch(0,0);
	waitKey(0);
	return 0;

}