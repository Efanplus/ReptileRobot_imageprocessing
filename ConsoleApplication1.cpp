// ConsoleApplication1.cpp: 定义控制台应用程序的入口点。
//


#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <opencv2/sfm/numeric.hpp>
#include <opencv2/sfm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

#define PI 3.1415926

using namespace cv;
using namespace std;
using namespace cv::sfm;

/*
@function 内参
*/
Mat cameraIntrinsicMatrix = (Mat_<double>(3, 3) << 1592.5,               1,          991.3,
                                                                                               	 0,      1602.4,          747.7,
	                                                                                             0,               0,                 1);
Mat cameraIntrinsicMatrix_inverse;
Mat RT;


bool counterssort(vector<Point> a, vector<Point> b) {
	return  contourArea(a, true) > contourArea(b, true);
}

 void Tsai_HandEye(Mat Hcg, vector<Mat> Hgij, vector<Mat> Hcij)
 {
 	CV_Assert(Hgij.size() == Hcij.size());
 	int nStatus = Hgij.size();
 
 	Mat Rgij(3, 3, CV_64FC1);
 	Mat Rcij(3, 3, CV_64FC1);
 
 	Mat rgij(3, 1, CV_64FC1);
 	Mat rcij(3, 1, CV_64FC1);
 
 	double theta_gij;
 	double theta_cij;
 
 	Mat rngij(3, 1, CV_64FC1);
 	Mat rncij(3, 1, CV_64FC1);
 
 	Mat Pgij(3, 1, CV_64FC1);
 	Mat Pcij(3, 1, CV_64FC1);
 
 	Mat tempA(3, 3, CV_64FC1);
 	Mat tempb(3, 1, CV_64FC1);
 
 	Mat A;
 	Mat b;
 	Mat pinA;
 
 	Mat Pcg_prime(3, 1, CV_64FC1);
 	Mat Pcg(3, 1, CV_64FC1);
 	Mat PcgTrs(1, 3, CV_64FC1);
 
 	Mat Rcg(3, 3, CV_64FC1);
 	Mat eyeM = Mat::eye(3, 3, CV_64FC1);
 
 	Mat Tgij(3, 1, CV_64FC1);
 	Mat Tcij(3, 1, CV_64FC1);
 
 	Mat tempAA(3, 3, CV_64FC1);
 	Mat tempbb(3, 1, CV_64FC1);
 
 	Mat AA;
 	Mat bb;
 	Mat pinAA;
 
 	Mat Tcg(3, 1, CV_64FC1);
 
 	for (int i = 0; i < nStatus; i++)
 	{
 		Hgij[i](Rect(0, 0, 3, 3)).copyTo(Rgij);
 		Hcij[i](Rect(0, 0, 3, 3)).copyTo(Rcij);
 
 		Rodrigues(Rgij, rgij);
 		Rodrigues(Rcij, rcij);
 
 		theta_gij = norm(rgij);
 		theta_cij = norm(rcij);
 
 		rngij = rgij / theta_gij;
 		rncij = rcij / theta_cij;
 
 		Pgij = 2 * sin(theta_gij / 2)*rngij;
 		Pcij = 2 * sin(theta_cij / 2)*rncij;
 		/************************************************************************/
 		/* 得到反对称矩阵                                                                     */
 		/************************************************************************/
 		tempA = sfm::skew(Pgij + Pcij);
 		tempb = Pcij - Pgij;
 
 		A.push_back(tempA);
 		b.push_back(tempb);
 	}
 
 	//Compute rotation
 	invert(A, pinA, DECOMP_SVD);
 
 	Pcg_prime = pinA * b;
 	Pcg = 2 * Pcg_prime / sqrt(1 + norm(Pcg_prime) * norm(Pcg_prime));
 	PcgTrs = Pcg.t();
	Mat tempsuanz = cv::sfm::skew(Pcg);
	//Mat tempsuanz;
 	Rcg = (1 - norm(Pcg) * norm(Pcg) / 2) * eyeM + 0.5 * (Pcg * PcgTrs + sqrt(4 - norm(Pcg)*norm(Pcg))*tempsuanz);
 
 	//Computer Translation 
 	for (int i = 0; i < nStatus; i++)
 	{
 		Hgij[i](Rect(0, 0, 3, 3)).copyTo(Rgij);
 		Hcij[i](Rect(0, 0, 3, 3)).copyTo(Rcij);
 		Hgij[i](Rect(3, 0, 1, 3)).copyTo(Tgij);
 		Hcij[i](Rect(3, 0, 1, 3)).copyTo(Tcij);
 
 
 		tempAA = Rgij - eyeM;
 		tempbb = Rcg * Tcij - Tgij;
 
 		AA.push_back(tempAA);
 		bb.push_back(tempbb);
 	}
 
 	invert(AA, pinAA, DECOMP_SVD);
 	Tcg = pinAA * bb;
 
 	Rcg.copyTo(Hcg(Rect(0, 0, 3, 3)));
 	Tcg.copyTo(Hcg(Rect(3, 0, 1, 3)));
 	Hcg.at<double>(3, 0) = 0.0;
 	Hcg.at<double>(3, 1) = 0.0;
 	Hcg.at<double>(3, 2) = 0.0;
 	Hcg.at<double>(3, 3) = 1.0;
 
 }


 float GetLineAngle(Point2f start, Point2f end) {
	 return atan2f(end.y - start.y, end.x - start.x);
 }

int main()
{
	/************************************************************************/
	/*
	@function: image processing
	*/
	Mat origin, origin_show;
	Mat origin_gray;
	Mat edge;
	//origin = imread("C:\\Users\\Efan\\Desktop\\a\\b.jpg");
	origin = imread("C:\\Users\\Efan\\Desktop\\frame.png");
	//origin = imread("C:\\Users\\Efan\\Desktop\\a\\chazuo11.png");
	/*resize(origin, origin, Size(), 0.5, 0.5);*/
	cvtColor(origin, origin, COLOR_BGR2GRAY);
	namedWindow("origin", WINDOW_NORMAL);
	imshow("origin", origin);
	Mat median_origin;
	/* 
	@medianBlur 中值滤波去除背景以及金属磨砂的噪声，用来中值处理的模板的维度要调整，这里选用的是11*11
	@substract 中值滤波的结果和原图像相减的操作相当于去除背景的操作，由于是8通道无符号数，所以结果小于0的数被截为0
	@opencv 中操作元素矩阵的方法？[i][j]是不行的
	*/
	medianBlur(origin,median_origin, 17);
	namedWindow("median", WINDOW_NORMAL);
	imshow("median", median_origin);
	Mat result_remove_background;
	subtract(median_origin, origin, result_remove_background);
	/*cout << result_remove_background.size().height << "," <<result_remove_background.size().width << endl;*/
	//cout << "result_remove_background = " << result_remove_background << endl;
	namedWindow("remove background", WINDOW_NORMAL);
	imshow("remove background", result_remove_background);
	/* 
	@OUST_threshold， 输入图像要是灰度图，如果没有进行灰度转化，可能会出现Assertion failed的错误
	*/
	/*
	double minv = 0.0, maxv = 0.0;
	double* minp = &minv;
	double* maxp = &maxv;
	minMaxIdx(result_remove_background, minp, maxp);
	cout << "Mat minv = " << minv << endl;
	cout << "Mat maxv = " << maxv << endl;
	*/
	Mat oust_suanz;
	double thresh;
	thresh = threshold(result_remove_background, oust_suanz, 0, 255, THRESH_OTSU);
	cout <<"thresh = "<< thresh << endl;
	threshold(result_remove_background, oust_suanz, thresh - 10, 255, THRESH_BINARY);
	//threshold(result_remove_background, oust_suanz, 4 , 255, CV_THRESH_BINARY);
	namedWindow("阈值分割后", WINDOW_NORMAL);
	imshow("阈值分割后", oust_suanz);
	imwrite("阈值分割后.png", oust_suanz);



	/************************************************************************/
	/* 
	@function:find circles, define the centers and define
	@find all the Contours
	@calculate the area of the every Contours and remove those whose area < 150, then the noise been removed
	@fill the contours and do edge again, then we get the circles
	@fitting circles and get their centers
	@attention: 找到的轮廓有三类，希望得到的特征圆，五个灯的轮廓，一般是其中几个，还有一些小的噪点，不同照片面积差距很大，希望可以实现自动分类
	@attention:  是不是可以尝试补光
	*/

	/************************************************************************
	@2019/01/02
	@1、利用聚类将所有的值为1的点分为11类，能不能做到一个圆是一类
	@2、利用轮廓的同级性做文章
	@3、还是利用轮廓的面积，先筛选出轮廓面积属于我们想要部分的9个轮廓或7-8个轮廓，然后画出其所在矩形（小矩形聚合成大矩形），之后将矩形之外的轮廓找出来，再画出剩余轮廓的所在矩形
	@第3点和第1点可行性比较高，第3点要想成功需要将矩阵缩小，矩阵的长和宽不能简单的横平竖直
	*/
	Point center[9];
	int n = 0;
	vector<vector<Point> > contours;
	vector<vector<Point> > contoursnoise;
	vector<Vec4i> hierarchy;
	//findContours(oust_suanz, contours, hierarchy,RETR_CCOMP, CHAIN_APPROX_NONE);
	findContours(oust_suanz, contours, RETR_CCOMP, CHAIN_APPROX_NONE);
	//sort(contours.begin(), contours.end(), counterssort);
	//vector<vector<Point>> ::iterator itc = contours.begin();
	vector<Point> tempV;
	Mat contoursImage(oust_suanz.rows, oust_suanz.cols, CV_8U, Scalar(0));
	for(int i = 0; i < contours.size(); i ++)
	{
		//获取轮廓的矩形边界
		Rect rect = boundingRect(contours[i]);
		int x = rect.x;
		int y = rect.y;
		int w = rect.width;
		int h = rect.height;
		

		//在oust_suanz上绘制矩形的轮廓边界
		//cv::rectangle(oust_suanz, rect, scalar(255), 1);


		//把轮廓面积不足100的区域放到contoursnoise中
		if (abs(contourArea(contours[i], true))< 50) {
			tempV.push_back(Point(x, y));
			tempV.push_back(Point(x, y + h));
			tempV.push_back(Point(x + w, y + h));
			tempV.push_back(Point(x + w, y));
			//cv::rectangle(oust_suanz, rect, scalar(255), 1);
			contoursnoise.push_back(tempV);
			cv::drawContours(oust_suanz, contoursnoise, -1, Scalar(0), FILLED);
		}
		else {
			std::cout << "————————————————————第" << i << "个孔轮廓" << contourArea(contours[i], true) << endl;
			drawContours(contoursImage, contours, i, Scalar(255), FILLED);
		}
		tempV.clear();
		//用黑色填充contoursnoise，从而删除这一部分面积

	}
	cv::namedWindow("去除小噪点", WINDOW_NORMAL);
	cv::imshow("去除小噪点", oust_suanz);
	cv::imwrite("去除小噪点.png", oust_suanz);
	cv::namedWindow("轮廓", WINDOW_NORMAL);
	cv::imshow("轮廓", contoursImage);
	contours.clear();
	vector<Point> forsuitarea;
	vector<Point> otherarea;
	findContours(contoursImage, contours, RETR_CCOMP, CHAIN_APPROX_NONE);
	Mat contoursImage_contours(oust_suanz.rows, oust_suanz.cols, CV_8U, Scalar(0));
	for (int i = 0; i < contours.size(); i++) {
		double g_dConArea = contourArea(contours[i],false);
		std::cout << "第" << i + 1 << "个轮廓" << g_dConArea << endl;
		if (g_dConArea > 300 && g_dConArea < 500) {
			drawContours(contoursImage_contours, contours, i, Scalar(255), FILLED);
			forsuitarea.insert(forsuitarea.end(), contours[i].begin(), contours[i].end());
		}
		else
		{
			otherarea.insert(otherarea.end(),contours[i].begin(), contours[i].end());
		}
	}

	cv::Mat color_contoursImage;
	//cvtColor(contoursImage, color_contoursImage, COLOR_GRAY2BGR);
	cvtColor(oust_suanz, color_contoursImage, COLOR_GRAY2BGR);

	//画出9个孔的轮廓
	RotatedRect ninehole;
	ninehole = minAreaRect(forsuitarea);
	Point2f rect[4];
	Point2f center_ninehole;
	center_ninehole = ninehole.center;
	ninehole.points(rect);
	for (int num = 0; num < 4; num++)
		line(color_contoursImage, rect[num], rect[(num + 1) % 4], Scalar(0,0,255), 2, 8);

	RotatedRect otherPS;
	otherPS = minAreaRect(otherarea);
	Point2f rectother[4];
	Point2f center_otherarea;
	center_otherarea = otherPS.center;
	otherPS.points(rectother);
	for (int num = 0; num < 4; num++)
		line(color_contoursImage, rectother[num], rectother[(num + 1) % 4], Scalar(0,255,0), 2, 8);
	
	cout << "两个中心点坐标分别为：" << center_ninehole << "------" << center_otherarea << endl;
	float angle_center = GetLineAngle(center_ninehole, center_otherarea)*180/PI;
	cout << "两个中心点连线与x轴的夹角为(-180 ~ 180)：" << angle_center << endl;

	float angle_ninearea = ninehole.angle;
	cout << "九孔区域最小矩形的旋转角度: " << angle_ninearea << endl;
	/****************************************************************************
	@attention: 使用两个矩形中心的连线作为一条辅助轴，与辅助轴成大概0度或者180度的为x轴
	@attention: 能不能使用矩阵的旋转角度
	@attention: center_ninehole和center_otherarea为两个矩阵中心，其格式为Point2f类型的
	*/
	cv::namedWindow("矩阵分区", WINDOW_NORMAL);
	cv::imshow("矩阵分区", color_contoursImage);

	cv::namedWindow("轮廓_轮廓", WINDOW_NORMAL);
	cv::imshow("轮廓_轮廓", contoursImage_contours);
	double thresh_Canny = 3;
	cv::Canny(contoursImage, edge, thresh_Canny, thresh_Canny * 3, 3);
	cv::namedWindow("edge", WINDOW_NORMAL);
	cv::imshow("edge", edge);
#if 0
	std::cout << "共有" << contours.size() << "个轮廓" << endl;
	int num_suanz = 1;
	for (int i = 0; i < contours.size(); i++) {
		double g_dConArea = contourArea(contours[i]);
		if (g_dConArea > 100) {
			std::cout << "第" << i + 1 << "个轮廓" << g_dConArea << endl;
		}
		if (g_dConArea > 200 && g_dConArea < 280)
		{
			std::cout << "————————————————————第" << num_suanz ++  << "个孔轮廓" << g_dConArea << endl;
			Mat pointsf;
			Mat(contours[i]).convertTo(pointsf, CV_32F);
			RotatedRect box = fitEllipse(pointsf);
			std::cout << box.center << endl;
			center[n] = box.center;
			n++;
		}
	}

	double thresh_Canny = 3;
 	Canny(contoursImage,edge,thresh_Canny,thresh_Canny*3,3);
 	namedWindow("edge", WINDOW_NORMAL);
 	imshow("edge", edge);
	Mat edgePlusCenter;
	cvtColor(edge, edgePlusCenter, COLOR_GRAY2RGB);
	for (int i = 0; i < 10; i++)
	{
		circle(edgePlusCenter, center[i], 5, Scalar(0,0,255));
	}
	namedWindow("得到特征点", WINDOW_NORMAL);
	imshow("得到特征点", edgePlusCenter);

	/************************************************************************/
	/* 
	@function 得到特征点的正确排序，与坐标系中的已知点对应起来
	@method /贴纸，小圆/检测黑色背景下边缘，检测直线/检测五个灯，考虑实际过程中灯亮的影响/实际过程中，利用机械臂的运动来检测
	*/
	/************************************************************************/



	/************************************************************************/
	/* 
	@function, 根据已知的特征点和其对应的世界坐标系的点得到其对应的H矩阵，并计算R,T
	*/

	/*
	@世界坐标系的点
	*/
	vector<Point2f> worldPoints;
	for (int Row = 1; Row <= 3; Row++)
	{
		for (int Col = 1; Col <= 3; Col++)
		{
			worldPoints.push_back(Point2f((Row - 2)*20, (Col - 2)*20));
		}
	}

	/*
	@存储图像上的特征点
	*/
	cout << "*********************************************************" << endl;
	for (int i = 0; i < 9; i++)
	{
		cout << center[i] << endl;
	}
	cout << "*********************************************************" << endl;
	vector<Point2f> corners;
	int suanz[] = {8,6,7,5,3,4,2,1,0};
	for (int i = 0; i < 9; i++)
	{
		cout << suanz[i] << endl;
		cout << center[suanz[i]] << endl;
		corners.push_back(Point2f(center[suanz[i]].x, center[suanz[i]].y));
	}
	/*
	@function 得到了单映射H矩阵，可以凭借此结果和已经知道的摄像机内参得到其外参
	@attention： 关于内参有一个问题就是u0，v0是摄像机光心的位置，在具体图像坐标系下和靶标坐标系下原点的选择造成的应该只是一个平移矩阵的问题
	@about findHomography: 这个函数得到的是第二个点系列corners到第一个点系列的worldPoints的单一性映射
	*/
	Mat h = findHomography(worldPoints, corners);
	invert(cameraIntrinsicMatrix, cameraIntrinsicMatrix_inverse);
	RT = cameraIntrinsicMatrix_inverse * h;
	cout << "RT" << endl << RT << endl;
	Mat  row1 = RT.col(0);
	Mat  row2 = RT.col(1);
	Mat  T = RT.col(2);
	cout << row1 << endl;
	cout << row2 << endl;
	Mat r1,r2;
	normalize(row1, r1);
	normalize(row2, r2);
	double p1 = norm(r1) / norm(row1);
	double p2 = norm(r2) / norm(row2);
	cout << "p1 && p2 && (p1+p2)/2:" << endl;
	cout << p1 << endl;
	cout << p2 << endl;
	cout << (p1 + p2) / 2 << endl;
	T = T * (p1+p2)/2;
// 	double x1, x2, x3;
// 	x1 = r1.at<double>(1, 0)*r2.at<double>(2, 0) - r1.at<double>(2, 0)*r2.at<double>(1, 0);
// 	x2 = r1.at<double>(2, 0)*r2.at<double>(0, 0) - r1.at<double>(0, 0)*r2.at<double>(2, 0);
// 	x3 = r1.at<double>(0, 0)*r2.at<double>(1, 0) - r1.at<double>(1, 0)*r2.at<double>(0, 0);
// 	Mat r3 = (Mat_<double>(3, 1) << x1, x2, x3);
	Mat r3 = r1.cross(r2);
	Mat R;
	Mat R_4x4;
	Mat suanz_4 = (Mat_<double>(1, 4) << 0, 0, 0, 1);
	hconcat(r1, r2, R);
	hconcat(R, r3, R);
	hconcat(R,T,R_4x4);
	vconcat(R_4x4, suanz_4, R_4x4);
	
	/*cout << R.size << endl;*/
	/************************************************************************/
	/* 
	@function 输出结果
	*/
	cout << "**************************************************************" << endl;
	cout << "摄像机图像坐标系和平板之间的转换矩阵如下：" << endl;
	cout << "R为：" << endl;
	cout << R << endl;
	cout << "T为：" << endl;
	cout << T << endl;
	cout << "4X4的姿态矩阵为：" << endl;
	cout << R_4x4 << endl;
	/************************************************************************/
	/* 
	@function ：检测H                                                                     */
	/*
	Mat A = (Mat_<double>(3, 1) <<20,20,1 );
	Mat a = h*A;
	cout << "a: image:" << center[8] << "world:" << a << endl;
	*/	
	/************************************************************************/	
	
#endif
	waitKey();
	return 1;
}