#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <math.h>
#include <string.h>

using namespace cv;
using namespace std;

//用来计算四边形的角度
static double angle( Point pt1, Point pt2, Point pt0 )
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

//设置绘制文本的相关参数
std::string text = "ok!";
int font_face = cv::FONT_HERSHEY_COMPLEX;
double font_scale = 2;
int thickness = 2;
int baseline;
//获取文本框的长宽
cv::Size text_size = cv::getTextSize(text, font_face, font_scale, thickness, &baseline);


int main(int /*argc*/, char** /*argv*/)
{
    VideoCapture cap("south.mp4");
    Mat image;
    int count=0;
    while(true)
    {
        cap>>image;

        //将图像转换为灰度图
        Mat gray,mask;
        cvtColor(image,gray,COLOR_BGR2GRAY);
        inRange(gray,30,150,mask);

        vector<vector<Point>> contours;
        vector<vector<Point> > squares;
        vector<Point> approx; //多边形逼近得到的点集
        vector<Rect> boundRect;//就共个左右共十个小矩形Rect类存放的位置
        vector<Rect> big_box;//不是小矩形,被排除掉的大矩形Rect存放的位置
        vector<double>big_box_area;//大矩形的面积
        vector<Vec4i> hierarchy;
        vector<Vec4i> big_hierarchy;

        findContours(mask, contours,hierarchy, RETR_TREE, CV_CHAIN_APPROX_SIMPLE);
        for(int i=0;i<contours.size();i++)
        {
            double length=arcLength(Mat(contours[i]), true);
            approxPolyDP(Mat(contours[i]), approx, length*0.02, true);
            Rect rect;
            rect= boundingRect(approx);
            double area=fabs(contourArea(Mat(approx)));

            if(approx.size()==4&&isContourConvex(Mat(approx))&&area<2500&&area>200)
            {
//                cout<<"rect_area: "<<area<<endl;
                double maxCosine=0;
                for(int j=2;j<5;j++)
                {
                    double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                    maxCosine = MAX(maxCosine, cosine);
                }
                if(maxCosine<0.3)
                {
                    if(rect.width>rect.height)
                    {
                       squares.push_back(approx);
                       boundRect.push_back(rect);
//                       cout<<"area: "<<area<<endl;
                    }
                }
            }
            //不是小矩形的存放
//            if(area>1000)
            else
            {
                big_box_area.push_back(area);
                big_box.push_back(rect);
                big_hierarchy.push_back(hierarchy[i]);
//               rectangle(image,rect,Scalar(255,0,0),8);
            }
        }

        //画出小矩形
        for(int i=0;i<squares.size();i++)
        {
            for (int k = 0; k < 4; k++)
            {
               line(image, squares[i][k], squares[i][(k + 1) % 4], Scalar(0, 0, 255), 2, 8);
            }
        }

        vector<Rect> left_block; //左边的小矩形存放位置
        vector<Rect> right_block;//右边的小矩形存放位置
        vector<Point> left_block_center;
        vector<Point> right_block_center;
        vector<Point> square_center;

        //把小矩形按左右去分开来
        for(int i=0;i<squares.size();i++)
        {
            double center_x=boundRect[i].tl().x+boundRect[i].width/2;
            double center_y=boundRect[i].tl().y+boundRect[i].height/2;
            Point center(center_x,center_y);
            circle(image,center,1,cv::Scalar(0, 255, 255),2);
            square_center.push_back(center);

            if(center_x<image.cols/2)
            {
                left_block.push_back(boundRect[i]);
                left_block_center.push_back(center);
            }
            if(center_x>image.cols/2)
            {
                right_block.push_back(boundRect[i]);
                right_block_center.push_back(center);
            }
        }

        //通过左右矩形的位置找到中间九宫格的位置
        Point top_left;
        Point bottom_right;
        double nine_block_width;
        double nine_block_height;
        Point Rectify_tl;
        Point Rectify_br;

        if(left_block.size()==5||right_block.size()==5||squares.size()==10)
        {

            double min_x,min_y,max_x,max_y;
            min_x=image.cols+100;
            min_y=image.rows+100;
            max_x=0;
            max_y=0;

            for(int i=0;i<square_center.size();i++)
            {
               if(square_center[i].x<min_x)
               {
                   min_x=square_center[i].x;
               }
               if(square_center[i].y<min_y)
               {
                   min_y=square_center[i].y;
               }
               if(square_center[i].x>max_x)
               {
                   max_x=square_center[i].x;
               }
               if(square_center[i].y>max_y)
               {
                   max_y=square_center[i].y;
               }
            }

            Point tl(min_x,min_y);
            top_left=tl;
            Point br(max_x,max_y);
            bottom_right=br;
            nine_block_width=max_x-min_x;
            nine_block_height=max_y-min_y;

            double tl_X=top_left.x+(nine_block_width*0.05);
            double tl_y=top_left.y-(nine_block_height*0.1);

            double br_x=bottom_right.x-nine_block_width*0.05;
            double br_y=bottom_right.y+nine_block_height*0.1;

            Point rectify_tl(tl_X,tl_y);
            Point rectify_br(br_x,br_y);
            Rectify_tl=rectify_tl;
            Rectify_br=rectify_br;

            rectangle(image,rectify_tl,rectify_br,Scalar(255,0,0),2); //圈出经过修正后九宫格的位置

/*
            //以下部分本来是想通过左右分类再找出中间九宫格的相对位置,发现鲁棒性不好,太严格了
            for(int i=0;i<left_block.size();i++)
            {
                for(int j=i;j<left_block.size();j++)
                {
                    if(left_block_center[i].y>left_block_center[j].y)
                    {
                        Point temp=left_block_center[i];
                        left_block_center[i]=left_block_center[j];
                        left_block_center[j]=temp;
                    }
                }
            }

            for(int i=0;i<right_block.size();i++)
            {
                for(int j=i;j<right_block.size();j++)
                {
                    if(right_block_center[i].y>right_block_center[j].y)
                    {
                        Point temp=right_block_center[i];
                        right_block_center[i]=right_block_center[j];
                        right_block_center[j]=temp;
                    }
                }
            }

            if(left_block.size()==5&&right_block.size()>2)
            {
                top_left=left_block_center[0];
                nine_block_height=left_block_center[4].y-left_block_center[0].y;
                nine_block_width=right_block_center[0].x-left_block_center[0].x;
                Point point(top_left.x+nine_block_width,top_left.y+nine_block_height);
                bottom_right=point;

            }


            else if(right_block.size()==5&&left_block.size()>2)
            {
                bottom_right=right_block_center[4];
                nine_block_height=right_block_center[4].y-right_block_center[0].y;
                nine_block_width=right_block_center[0].x-left_block_center[0].x;
                Point point(bottom_right.x-nine_block_height,bottom_right.y-nine_block_height);
                top_left=point;
            }



            double tl_X=top_left.x+(nine_block_width*0.05);
            double tl_y=top_left.y-(nine_block_height*0.1);

            double br_x=bottom_right.x-nine_block_width*0.05;
            double br_y=bottom_right.y+nine_block_height*0.1;

            Point2f rectify_tl(tl_X,tl_y);
            Point2f rectify_br(br_x,br_y);
            rectangle(image,rectify_tl,rectify_br,Scalar(255,0,0),2);
*/
        }

        //在选定的范围内找出九宫格
        vector<Rect> final_box;
        vector<Point>final_box_center;
        for(int i=0;i<big_box.size();i++)
        {
            double center_x=big_box[i].tl().x+big_box[i].width/2;
            double center_y=big_box[i].tl().y+big_box[i].height/2;
            Point center(center_x,center_y);
            if(center_x>Rectify_tl.x&&center_x<Rectify_br.x&&center_y>Rectify_tl.y&&center_y<Rectify_br.y&&big_box_area[i]<7000&&big_box_area[i]>2000)
            {
               final_box.push_back(big_box[i]);
               final_box_center.push_back(center);
               circle(image,center,2,Scalar(0,0,255),2);
                cout<<"big_box_area[i]: "<<big_box_area[i]<<endl;
            }
        }

        //固定就宫格的顺序
        if(final_box.size()==9)
        {
            for(int i=0;i<9;i++)
            {
                rectangle(image,final_box[i].tl(),final_box[i].br(),Scalar(0,255,255),2);
            }
            //按张y从小到大排序
            for(int i=0;i<final_box.size();i++)
            {
                for(int j=i+1;j<final_box.size();j++)
                {
                    if(final_box_center[i].y>final_box_center[j].y)
                    {
                        Point temp=final_box_center[i];
                        final_box_center[i]=final_box_center[j];
                        final_box_center[j]=temp;
                    }
                }
            }

            for(int i=0;i<3;i++)
            {
                for(int j=i+1;j<3;j++)
                {
                    if(final_box_center[i].x>final_box_center[j].x)
                    {
                        Point first=final_box_center[i];
                        final_box_center[i]=final_box_center[j];
                        final_box_center[j]=first;
                    }
                }
            }

            for(int i=3;i<6;i++)
            {
                for(int j=i+1;j<6;j++)
                {
                    if(final_box_center[i].x>final_box_center[j].x)
                    {
                        Point temp=final_box_center[i];
                        final_box_center[i]=final_box_center[j];
                        final_box_center[j]=temp;
                    }
                }
            }

            for(int i=6;i<9;i++)
            {
                for(int j=i+1;j<9;j++)
                {
                    if(final_box_center[i].x>final_box_center[j].x)
                    {
                        Point temp=final_box_center[i];
                        final_box_center[i]=final_box_center[j];
                        final_box_center[j]=temp;
                    }
                }
            }

            for(int i=0;i<final_box.size();i++)
            {
                 std::string number = to_string(i+1);
                 cv::putText(image, number, final_box_center[i], font_face, font_scale, cv::Scalar(0, 255, 255), thickness, 8, 0);
            }
        }

        count++;
//        cout<<"conut: "<<count<<endl;
        imshow("gray",gray);
        imshow("image",image);
        imshow("mask",mask);
//        imshow("nine_block",nine_block);
        int wait=10;
        if(final_box.size()!=9)
        {
            wait=0;
        }
        waitKey(wait);
    }
    return 0;
}
