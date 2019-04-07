#include "stdafx.h"
#include "TLR.h"
#include <iostream>
#include <fstream>
#include <string> 

TLR::TLR(void)
{
	
}

TLR::~TLR(void)  //相当于直接调用这个函数，应该是先清空释放内存的意思？
{
}

void TLR::findHSV(Mat& RGB, Mat& mask, Mat& state_map)
{
	Mat HSV;
	unsigned char h, s, v;
	cvtColor(RGB, HSV, CV_BGR2HSV);  //rgb原图转化为hsv图

	for (int i = 0; i < HSV.rows; i++){
		for (int j = 0; j < HSV.cols; j++)
		{
			h = HSV.at<Vec3b>(i, j)[0];
			s = HSV.at<Vec3b>(i, j)[1];
			v = HSV.at<Vec3b>(i, j)[2];

			if ((h<H_R1 || h>H_R2) && s>S_TH && v>V_TH)
			{
				mask.at<uchar>(i, j) = 255;
				state_map.at<uchar>(i, j) = 255;
			}

			if (h > H_G1 && h<H_G2 && s>S_TH && v > V_TH)
			{
				mask.at<uchar>(i, j) = 255;
				state_map.at<uchar>(i, j) = 128;
			}


		}
	}
}

//进行形态学操作，开到闭，或者闭到开  //用形态学操作进行滤波，去除噪音
void TLR::Morph(Mat& src, Mat& dst, int opWidth, int opHeight, int clWidth, int clHeight, int model)
{
	Mat open_element = getStructuringElement(MORPH_ELLIPSE, Size(opWidth, opHeight));  //这个定义的是卷积核的大小
	Mat close_element = getStructuringElement(MORPH_ELLIPSE, Size(clWidth, clHeight));

	Mat open, close;
	if (model==1)  //model是1，先开再闭（这个方法牛逼）
	{
		morphologyEx(src, open, MORPH_OPEN, open_element);
		morphologyEx(open, dst, MORPH_CLOSE, close_element);
	}
	else if (model==0)  //闭操作
	{
		morphologyEx(src, dst, MORPH_CLOSE, close_element);
		//morphologyEx(close, dst, MORPH_OPEN, open_element);
		//imshow("kai", open);
		imshow("yahaha", dst);
		waitKey(0);
	}

	else if (model == 2)  //pengzhang
	{
		morphologyEx(src, dst, MORPH_DILATE, close_element);
	}
	else if (model == 3)
	{
	//	morphologyEx(src, open, MORPH_DILATE, close_element);
		morphologyEx(src, dst, MORPH_CROSS, close_element);
	}

}

//寻找信号灯轮廓（mask里面）
void TLR::findBulb(Mat& mask,vector<Rect>& bulb)
{
	vector<vector<Point>> contours;  //用来保存找到的轮廓
	vector<Vec4i> hierarchy; //定义层级，里面储存四个向量int
	//imshow("mask1", mask);
	findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);  //只检查最外围轮廓，仅保存拐点信息。这时候show出来的话就是点的图

	vector<vector<Point>> conPoints(contours.size());  //浅拷贝两个向量（那么图片向量是不是也可以直接这样浅拷贝）
	vector<Rect> boundRect(contours.size());

	int max = 0;
	int min = 0;
	float aspectRatio = .0f;

	for (size_t i = 0; i != contours.size(); ++i)  //迭代每一个轮廓
	{
		//drawContours(mask, contours, i, 255, 1, 8, vector<Vec4i>(), 0, Point());//这时候showmask就是轮廓图（把点的信息变成了轮廓信息）

		boundRect[i] = boundingRect(Mat(contours[i]));   //boundingRect 找到一个矩形框来框这个轮廓
		//rectangle(mask, cvPoint(100, 150), cvPoint(150, 500), Scalar(0, 0, 255), 2, 8, 0);


		min = MIN(boundRect[i].height, boundRect[i].width);
		max = MAX(boundRect[i].height, boundRect[i].width);

		aspectRatio =(float)max /(float)min;

		if (MIN_ASPECT<aspectRatio < MAX_ASPECT && max<MAX_WIDTH && min>MIN_WIDTH)
		{
			bulb.push_back(boundRect[i]);  //把高宽比符合要求的框加到了bulb里面去
			
		}
		
	}
}

void TLR::findBoxShape(Mat& src, Mat& mask, vector<Rect>& bulb)
{
	vector<vector<Point>> contours;  //用来保存找到的轮廓
	vector<Vec4i> hierarchy; //定义层级，里面储存四个向量int
	//imshow("mask1", mask);
	findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);  //只检查最外围轮廓，仅保存拐点信息。这时候show出来的话就是点的图

	vector<vector<Point>> conPoints(contours.size());  //浅拷贝两个向量（那么图片向量是不是也可以直接这样浅拷贝）
	vector<Rect> boundRect(contours.size());
	vector<vector<Point>>hull(contours.size());

	int max = 0;
	int min = 0;
	float aspectRatio = .0f;
	float hullRatio = .0f;

	for (size_t i = 0; i != contours.size(); ++i)  //迭代每一个轮廓
	{

		convexHull(Mat(contours[i]), hull[i], false);  //找到轮廓的凸包
		boundRect[i] = boundingRect(Mat(contours[i]));   //boundingRect 找到一个矩形框来框这个轮廓
		
		

		/*min = MIN(boundRect[i].height, boundRect[i].width);
		max = MAX(boundRect[i].height, boundRect[i].width);*/
		hullRatio = ((float)contourArea(contours[i])) / ((float)contourArea(hull[i]));
		//aspectRatio = (float)max / (float)min;
		aspectRatio = (float)boundRect[i].height / (float)boundRect[i].width;

		if (MIN_ASPECT<aspectRatio && aspectRatio < MAX_ASPECT && hullRatio>HU_TH && boundRect[i].width>MIN_WIDTH)
		{
			bulb.push_back(boundRect[i]);  //把高宽比符合要求的框加到了bulb里面去

		}
		/*for (size_t i = 0; i!= bulb.size(); ++i)
		{
		rectangle(src, bulb[i].tl(), bulb[i].br(), Scalar(0, 0, 255), 2, 8, 0);

		}*/
	}
	
}

//	//以下为画出来这个框
//	Mat drawing = mask.clone();
//	/*for (int i = 0; i< contours.size(); i++)
//	{
//		rectangle(drawing,boundRect[i].tl(), boundRect[i].br(), 255, 2, 8, 0);
//
//	}
//*/
//	/// 显示在一个窗口  
	/*imshow("Contours", mask);
	waitKey(0);
*/



//用RGB颜色阈值找到信号灯的框的黑色部分+HSV找到框的灯的部分
void TLR::findBox(Mat& src, Mat& box_mask,Mat& mask,Mat& state_map)
{
	unsigned char r, g, b;
	for (int i = 0; i < src.rows; i++){
		for (int j = 0; j < src.cols; j++)
		{
			b = src.at<Vec3b>(i, j)[0];
			g = src.at<Vec3b>(i, j)[1];
			r = src.at<Vec3b>(i, j)[2];

			if (MAX(r, g, b) < BOX_TH1)
			//if (r+g+b<BOX_TH1)
			{
				box_mask.at<uchar>(i, j) = 255;
			}
		}
	}
	Mat HSV;
	unsigned char h, s, v;
	cvtColor(src, HSV, CV_BGR2HSV);  //rgb原图转化为hsv图

	for (int i = 0; i < HSV.rows; i++){
		for (int j = 0; j < HSV.cols; j++)
		{
			h = HSV.at<Vec3b>(i, j)[0];
			s = HSV.at<Vec3b>(i, j)[1];
			v = HSV.at<Vec3b>(i, j)[2];

			if ((h<H_R1 || h>H_R2) && s>S_TH && v>V_TH)
			{
				mask.at<uchar>(i, j) = 255;
				box_mask.at<uchar>(i, j) = 255;
				state_map.at<uchar>(i, j) = 255;
			}

			if (h > H_G1 && h<H_G2 && s>S_TH && v > V_TH)
			{
				mask.at<uchar>(i, j) = 255;
				box_mask.at<uchar>(i, j) = 255;
				state_map.at<uchar>(i, j) = 255;
			}


		}
	}
	
}

//判断这个区域的颜色信息
void TLR::findState(Mat& state_map, Rect& bulb, int& State)
{
	int count_red = 0, count_green = 0;
	int flag=0;
	for (int i = bulb.y; i < (bulb.y + bulb.height);++i)
	{
		for (int j = bulb.x; j < (bulb.x + bulb.width);++j)
		{
			if (state_map.at<uchar>(i, j) == 255)
			{
				State = 1;
				count_red++;
				flag = 1;
				break;
			}
			else if (state_map.at<uchar>(i, j) == 128)
			{
				State = 2;
				count_green++;
				flag = 1;
				break;
			}
		}
		if (flag == 1) break;
	}

	//if (count_red > ((bulb.height*bulb.width) /3))
	//{
	//	State = 1;  //红色设定为1
	//}
	//else if (count_green > (bulb.height*bulb.width) / 3)
	//{
	//	State = 2;
	//}
	//else State = 0;

}

void TLR::showRect(Mat& state_map,Mat& src, vector<Rect>& bulb)
{
	for (size_t i = 0; i != bulb.size(); ++i)
	{
		String color;
		int State = 0;
		findState(state_map, bulb[i],State);
		if (State == 1)//红色的时候
		{
			color = "RED";
			rectangle(src, bulb[i].tl(), bulb[i].br(), Scalar(0, 0, 255), LINE_THICK1, 8, 0);
		}
		else if (State == 2)
		{
			color = "GREEN";
			rectangle(src, bulb[i].tl(), bulb[i].br(), Scalar(0, 255, 0), LINE_THICK1, 8, 0);
		}
		//else
		//{
		//	color = nullptr;//空指针
		//}

		//putText
	}
}

void TLR::RecognitionLight(Mat& src,Mat& dst)
{
	Mat roi = src(Range(0, src.rows/2 ), Range(0, src.cols));
	Mat mask(roi.size(), CV_8UC1, Scalar(0));
	Mat state_map(roi.size(), CV_8UC1, Scalar(0));
	findHSV(roi, mask, state_map);
	int opWidth = 5;
	int opHeight = 5;
	int clWidth = 9;
	int clHeight = 9;
	Morph(mask, mask, opWidth, opHeight, clWidth, clHeight,1);

	vector<Rect> bulb;
	bulb.resize(0);
	findBulb(mask, bulb);
	//for (size_t i = 0; i != bulb.size(); ++i){
	//		rectangle(dst, bulb[i].tl(), bulb[i].br(), Scalar(0, 0, 255), 2, 8, 0);
	//	}
	//	//imshow("开到闭", dst);

	showRect(state_map, dst, bulb);



}

//分割数据集中的信号灯数据
void TLR::dataset(void)
{
	ifstream document("F:\\biaozhu\\label\\picss.txt");
	string s;
	int count = 0;
	int NumGC = 0;
	int NumGL = 0;
	int NumGS = 0;
	int NumGR = 0;
	int NumRC = 0;
	int NumRL = 0;
	int NumRS = 0;
	int NumRR = 0;
	while (document.peek()!=EOF)
	{
		getline(document, s);
		string Img_name=s.substr(20,19)+".bmp";//第二个参数是所需字符串的长度
	    ifstream infile_feat(s); //加载数据文件  
		string feature; //存储读取的每行数据  
		float feat_onePoint;  //存储每行按空格分开的每一个float数据  
		vector<float> lines; //存储每行数据  
		vector<vector<float>> lines_feat; //存储所有数据  
		lines_feat.clear();
		int counter = 0;
		while (infile_feat.peek()!=EOF)
		{
			getline(infile_feat, feature); //一次读取一行数据  
			stringstream stringin(feature); //使用串流实现对string的输入输出操作  
			lines.clear();
			while (stringin >> feat_onePoint) {      //按空格一次读取一个数据存入feat_onePoint   
				lines.push_back(feat_onePoint); //存储每行按空格分开的数据   
			}
			lines_feat.push_back(lines); //存储所有数据  存在一个二维数组里面
		}

		for (int i = 0; i < lines_feat.size(); i++)
		{
			if (lines_feat[i][0] == 3 || lines_feat[i][1] == 5 || lines_feat[i][1] == 6 || lines_feat[i][1] == 7 || lines_feat[i][2] == 2 || lines_feat[i][5]<15
				|| lines_feat[i][5]>lines_feat[i][6] || (lines_feat[i][6] / lines_feat[i][5]<2))
			{
				break;
			}
			else
			{
				//切割图片
				count++;
				//Mat src = imread("F:\\OPENCV\\3\\LightRaw000375.jpg", -1);
				Mat src = imread("I:\\TL\\"+Img_name,-1);
				/*imshow("src", src);
				cvWaitKey(0);
				break;*/
				Mat roi = src(Range(lines_feat[i][4], lines_feat[i][4] + lines_feat[i][6]), Range(lines_feat[i][3], lines_feat[i][3] + lines_feat[i][5]));
				/*imwrite("F:\\biaozhu\\ML2\\" + to_string(count) + ".bmp", roi);
				cout <<Img_name<< ":ok" << endl;*/


				//生成txt 标注文件
				string label;
				

				if (lines_feat[i][0] == 2 && lines_feat[i][1] == 1)
				{
					NumGC++;
					label = "GC";
					if (NumGC <= 190)
					imwrite("F:\\biaozhu\\ML2\\" + label +to_string(NumGC) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				else if (lines_feat[i][0] == 2 && lines_feat[i][1] == 2)
				{
					NumGL++;
					label = "GL";
					if (NumGL <= 190)
					imwrite("F:\\biaozhu\\ML2\\" + label + to_string(NumGL) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				else if (lines_feat[i][0] == 2 && lines_feat[i][1] == 4)
				{
					NumGS++;
					label = "GS";
					if (NumGS <= 190)
					imwrite("F:\\biaozhu\\ML2\\" + label + to_string(NumGS) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				else if (lines_feat[i][0] == 2 && lines_feat[i][1] == 3)
				{
					NumGR++;
					label = "GR";
					if (NumGR<=190)
					imwrite("F:\\biaozhu\\ML2\\" + label + to_string(NumGR) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				else if (lines_feat[i][0] == 1 && lines_feat[i][1] == 1)
				{
					NumRC++;
					label = "RC";
					if (NumRC <= 190)
					imwrite("F:\\biaozhu\\ML2\\" + label + to_string(NumRC) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				else if (lines_feat[i][0] == 1 && lines_feat[i][1] == 2)
				{
					NumRL++;
					label = "RL";
					if (NumRL <= 190)
					imwrite("F:\\biaozhu\\ML2\\" + label + to_string(NumRL) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				else if (lines_feat[i][0] == 1 && lines_feat[i][1] == 4)
				{
					NumRS++;
					label = "RS";
					if (NumRS <= 190)
					imwrite("F:\\biaozhu\\ML2\\" + label + to_string(NumRS) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				else if (lines_feat[i][0] == 1 && lines_feat[i][1] == 3)
				{
					NumRR++;
					label = "RR";
					if (NumRR <= 190)
					imwrite("F:\\biaozhu\\ML2\\" + label + to_string(NumRR) + ".bmp", roi);
					cout << Img_name << ":ok" << endl;
				}
				////加标签部分，考虑到是knn不用这部分
				//ofstream labelfile;
				//labelfile.open("F:\\biaozhu\\ML2\\" + to_string(count) + ".txt");
				//labelfile << label;
				//labelfile.close();
				
			}
		}
		infile_feat.close();
		ofstream tol;
		tol.open("F:\\biaozhu\\ML2\\tol.txt");
		tol << "GreenCircular:" << to_string(NumGC) << '\n' << "GreenLeft:" << to_string(NumGL) << '\n' << "GreenStraight:" << to_string(NumGS) << '\n' << "GreenRight:" << to_string(NumGR) << '\n'
			<< "RedCircular:" << to_string(NumRC) << '\n' << "RedLeft:" << to_string(NumRL) << '\n' << "RedStraight:" << to_string(NumRS) << '\n' << "RedRight:" << to_string(NumRR) << endl;
		tol.close();
		

	}
}


//切割框选中的感兴趣区域 放入候选区域的结构里面
void TLR::roiPic(Mat& src, vector<candidate>& roiPics,vector<Rect>& bulb)
{
	roiPics.resize(bulb.size());
	for (size_t i = 0; i != bulb.size(); ++i)
	{
		
		roiPics[i].candPics = Mat(src, bulb[i]);
		roiPics[i].candRect = bulb[i];

		
	}
	
}


//根据颜色分割和位置特征。得到信号灯的候选区域，并且判断了信号灯的状态，存在了struct里面（红，绿）
void TLR::candJud(vector<candidate>& roiPics, vector<candidate>& candPics)
{

	for (size_t i = 0; i != roiPics.size(); ++i){

		vector<Rect> bulb;

		Mat mask(roiPics[i].candPics.size(), CV_8UC1, Scalar(0));
		Mat state(roiPics[i].candPics.size(), CV_8UC1, Scalar(0));
		findHSV(roiPics[i].candPics, mask, state); //用HSV在小模板上分割出颜色(已经大小相等)
		findBulb(mask, bulb);  //找到灯内的框
		
		if (bulb.size()==1)    
		{
			if (bulb[0].height < roiPics[i].candRect.height / 2)
			{
				//这里考虑不用位置信息判断而是都放进去//2-红色
				/*if (bulb[0].y < roiPics[i].candRect.height / 2){
					roiPics[i].State = 2;*/
					candPics.push_back(roiPics[i]);

					
				//}
				//else{//1-绿色
				//	roiPics[i].State = 1;
				//	candPics.push_back(roiPics[i]);
				//}
			}
		}
		

	}
}


void TLR::Recognition(Mat& src, Mat& dst)
{
	int opWidth = 3;
	int opHeight = 3;
	int clWidth = 3;
	int clHeight = 3;
	Mat roi_img = src(Range(0, src.rows / 2), Range(0, src.cols));
	//找框的mask
	Mat box_map(roi_img.size(), CV_8UC1, Scalar(0));
	Mat morph_map(roi_img.size(), CV_8UC1, Scalar(0));
	//找灯的mask
	Mat mask(roi_img.size(), CV_8UC1, Scalar(0));
	Mat state_map(roi_img.size(), CV_8UC1, Scalar(0));
	vector<Rect> bulb;
	vector<candidate> roi, cand;
	//找到框
	findBox(roi_img, box_map,mask,state_map);
	//形态学操作
	Morph(box_map, morph_map, opWidth, opHeight, clWidth, clHeight, 1);
	//最终找到框的候选区域
	findBoxShape(dst, morph_map, bulb);
	roiPic(dst, roi, bulb);
	candJud(roi, cand);
	
		for (size_t i = 0; i < cand.size(); ++i)
		{
			Mat dst(ROI_HEIGHT, ROI_WIDTH, CV_8UC3, Scalar(0, 0, 0));
			resize(cand[i].candPics, dst, dst.size(), 0, 0);  //归一化大小
			Mat mask(dst.size(), CV_8UC1, Scalar(0));
			Mat state(dst.size(), CV_8UC1, Scalar(0));
			findHSV(dst, mask, state); //用HSV在小模板上分割出颜色(已经大小相等)
			

			float result = practice(mask);
			if (result == 2)
			{
				cand[i].State = 2;
			}
			if (result == 1)
			{
				cand[i].State = 1;
			}
			if (result == 3)
			{
				cand[i].State = 3;
			}

			//cout << cand[i].State << endl;
		}

		showBox(dst, cand);


}


void TLR::showBox(Mat& src, vector<candidate>& cand)
{
	for (size_t i = 0; i != cand.size(); ++i)
	{
		String color;
		if (cand[i].State == 2)//红色的时候
		{
			color = "RED";
			rectangle(src, cand[i].candRect.tl(), cand[i].candRect.br(), Scalar(0, 0, 255), LINE_THICK2, 8, 0);
		}
		else if (cand[i].State == 1)
		{
			color = "GREEN";
			rectangle(src, cand[i].candRect.tl(), cand[i].candRect.br(), Scalar(0, 255, 0), LINE_THICK2, 8, 0);
		}
		else if (cand[i].State == 3)
		{
			color = "GREENLEFT";
			rectangle(src, cand[i].candRect.tl(), cand[i].candRect.br(), Scalar(255, 0, 0), LINE_THICK2, 8, 0);
		}
	}
}




void TLR::resizeImg(void)
{
	ifstream document("F:\\biaozhu\\ML2\\pics.txt");
	string s;
	while (document.peek() != EOF)
	{
		getline(document, s);
		string name=s.substr(18);  //后一位默认到结束
		cout << name << endl;
		Mat dst(ROI_HEIGHT, ROI_WIDTH, CV_8UC3, Scalar(0, 0, 0));
		Mat src = imread(s, -1);
		resize(src, dst, dst.size(), 0, 0);   //默认使用双线性差值,
		/*imshow("yahaha", dst);
		waitKey(0);*/
		imwrite("F:\\biaozhu\\成功图\\"+name, dst);
	}
}




//////////////////////机器学习部分
//初始化机器学习参数
void TLR::kNclassifer(void)
{
	//训练用的数量的种类
	train_samples = 9;
	classes = 3;
	//训练用的同一图片大小
	size_height = 45;
	size_width = 20;
	//生成训练用数据矩阵
	trainData.create(train_samples*classes, size_height*size_width, CV_32FC1);  //取兴趣区域
	//trainData.create(train_samples*classes, size_height*size_width, CV_32FC1);
	trainClasses.create(train_samples*classes, 1, CV_32FC1);

	/*printf(" ------------------------------------------------------------------------\n");
	printf("|\t识别结果\t|\t 测试精度\t|\t  准确率\t|\n");
	printf(" ------------------------------------------------------------------------\n");
	getData();
	train();
	test();*/
}

void TLR::getData(void)
{
	Mat src;
	int i, j;
	for (i = 0; i < classes; i++){
		for (j = 0; j < train_samples; j++)
		{
			
			//string filename = "F:\\biaozhu\\成功图\\" +  to_string(i + 1) + "\\" + to_string(j + 1) + ".bmp";
			string filename = "F:\\biaozhu\\箭头实验\\" + to_string(i + 1) + "\\" + to_string(j + 1) + ".bmp";
			//cout << filename << endl;
			src = imread(filename,0);  //这里是0转灰度图
			if (src.empty())
			{
				cout << "Error: Cant load image"<<endl;
			}
			//Mat roi = src(Range((src.rows*2) / 3, src.rows), Range(0, src.cols));
			Mat dst;
			Mat dst1(src);
			dst1.convertTo(dst, CV_32FC1);
			float* data1 = trainData.ptr<float>(i*train_samples + j);//指向一行   //取矩阵的首地址

			for (int k = 0; k < dst.rows; k++)
			{
				float* data2 = dst.ptr<float>(k);
				for (int l = 0; l < dst.cols; l++)
				{
					data1[k*dst.cols+l] = data2[l];
				}
			}
			
            ////设置label的值
			trainClasses.at<float>(i*train_samples + j, 0) = i+1;

			
			
		}
	}
}

void TLR::train(void)
{
	knn = new KNearest(trainData, trainClasses, Mat(), false, K);
}

float TLR::classify(Mat& image)
{

	float result;
	Mat tem1, tem2;
	Mat Nearest;
	Nearest.create(1, K, CV_32FC1);
	result = knn->find_nearest(image, K, tem1, Nearest, tem2);

	int accuracy = 0;
	for (int i = 0; i<K; i++)
	{
		if (Nearest.at<float>(0, i) == result)//投票率
			accuracy++;
	}
	float pre = 100 * ((float)accuracy / (float)K);

	printf("|\t    %.0f    \t| \t    %.2f%%  \t| \t %d of %d \t| \n", result, pre, accuracy, K);
	printf(" ------------------------------------------------------------------------\n");

	return result;
}

void TLR::test(void)
{
	Mat src;
	//IplImage prs_image;
	Mat row, data;
	int error = 0;
	int testCounter = 0;

	for (int i = 0; i < classes; i++){
		for (int j = 100; j < 60+train_samples; j++)
		{
			string filename = "F:\\biaozhu\\成功图\\" + to_string(i + 1) + "\\" + to_string(j + 1) + ".bmp";
			src = imread(filename, 0);
			if (src.empty())
			{
				cout << "Error: Cant load image" << endl;
			}
			//Mat roi = src(Range((src.rows*2) / 3, src.rows), Range(0, src.cols));
			Mat dst;
			//Mat dst2(1, (size_height*size_width)/3, CV_32FC1);
			Mat dst2(1, size_height*size_width, CV_32FC1);
			src.convertTo(dst, CV_32FC1);

			float* data1 = dst2.ptr<float>(0);//指向一行   //取矩阵的首地址

			for (int k = 0; k < dst.rows; k++)
			{
				float* data2 = dst.ptr<float>(k);
				for (int l = 0; l < dst.cols; l++)
				{
					data1[k*dst.cols + l] = data2[l];
				}
			}
			//cout << dst << endl;
			
			float result=classify(dst2);  //指向预测的结果
			if ((int)result != i+1)
				error++;
			testCounter++;
		}
	}

	float totalerror = 100 * (float)error / (float)testCounter;
	printf("测试系统误识率: %.2f%%\n", totalerror);
}


float TLR::practice(Mat& image)
{
	//cout << image << endl;
	Mat row, data;
	Mat dst;
	Mat dst2(1, size_height*size_width, CV_32FC1);
	Mat dst1(image);
	//Mat roi = image(Range((image.rows * 2) / 3, image.rows), Range(0, image.cols));
	dst1.convertTo(dst, CV_32FC1);
	float* data1 = dst2.ptr<float>(0);   //取矩阵的首地址

	for (int k = 0; k < dst.rows; k++)
	{
		float* data2 = dst.ptr<float>(k);
		for (int l = 0; l < dst.cols; l++)
		{
			data1[k*dst.cols + l] = data2[l];
		}
	}
	float result = classify(dst2);
	return result;
}