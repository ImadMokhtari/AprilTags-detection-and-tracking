﻿#include "features_tracking.h"
#include "tag_detection_features.h"
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "apriltag.h"
#include "tag36h11.h"
#include "common/getopt.h"
#include "pose_estimation.h"
#include <mutex>

using namespace cv;
using namespace std;

Mat src_gray,prevgray;
vector<Point> tag_points;
vector<Point2f> corners,corners0,next_corners;
bool Find_detec=0,flag,not_detect=false,begin_detect=true;;
int size_c;
mutex cap_mutex;
int siz;

char video1[] = "/home/imad/Desktop/Mini Projet Soft/Video/carree.webm";

VideoCapture cap(1);
// Define the codec and create VideoWriter object.The output is stored in 'outcpp.avi' file.

int frame_width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
int frame_height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

VideoWriter video("/home/imad/Desktop/evaluation/detect_track.avi",CV_FOURCC('M','J','P','G'),10, Size(frame_width,frame_height));
void *detectionv(void *i);
void *trackingv(void *i);

int main()
{


    int rc,rct;
    pthread_t detection,tracking;
    cap.open(1);
    //sleep(1);
    rc = pthread_create(&detection, NULL, detectionv, (void*)1);
    if (rc)
    {
        cout << "Error:unable to create thread," << rc << endl;
        exit(-1);
    }

    rct = pthread_create(&tracking, NULL, trackingv, (void*)1);
    if (rct) {
        cout << "Error:unable to create thread," << rc << endl;
        exit(-1);
    }

    pthread_join(tracking,NULL);
    pthread_exit(NULL);
    return(0);
}

vector<Point2f> corners1,box_edges;
void *detectionv(void *i)
{

    Tag_Detection_Features Tag;
    Mat  gray,frame;
    zarray_t *detections;
    apriltag_detection_t *det;
    bool detec=0,grab;

    if (!cap.isOpened())
        cerr << "Couldn't open video capture device12" << endl;

    getopt_t *getopt = getopt_create();
    apriltag_family_t *tf = NULL;
    tf = tag36h11_create();
    apriltag_detector_t *td = apriltag_detector_create();
    Tag.Tag_Define(getopt,tf,td);
  //  ofstream myfile_detections;
    //myfile_detections.open("/home/imad/Desktop/evaluation/e1.csv");
    //myfile_detections<<"detectx1"<<","<<"detecty1"<<","<<"detect2"<<","<<"detect2"<<","<<"detectx3"<<","<<"detecty3"<<","<<"detectx4"<<","<<"detecty4"<<","<<"\n";

    while(1)
    {
        detec=1;
        if(begin_detect)
        {
            while (detec)
            {
                //cap_mutex.lock();
                cap >>  frame;
                //   cap_mutex.unlock();
                if(!frame.empty())
                {
                    cvtColor(frame, gray, COLOR_BGR2GRAY);
                    image_u8_t im = { .width = gray.cols,
                                      .height = gray.rows,
                                      .stride = gray.cols,
                                      .buf = gray.data
                                    };
                    detections = apriltag_detector_detect(td, &im);
                    siz=zarray_size(detections);
                    for (int i = 0; i < siz; i++)
                    {
                        zarray_get(detections, i, &det);
                        tag_points.clear();
                        tag_points.push_back(Point(det->p[0][0], det->p[0][1]));
                        tag_points.push_back(Point(det->p[1][0], det->p[1][1]));
                        tag_points.push_back(Point(det->p[2][0], det->p[2][1]));
                        tag_points.push_back(Point(det->p[3][0], det->p[3][1]));
                        box_edges.clear();
                        box_edges.push_back(Point2f(det->p[0][0], det->p[0][1]));
                        box_edges.push_back(Point2f(det->p[1][0], det->p[1][1]));
                        box_edges.push_back(Point2f(det->p[2][0], det->p[2][1]));
                        box_edges.push_back(Point2f(det->p[3][0], det->p[3][1]));
                      //  myfile_detections<<box_edges.at(0).x<<","<<box_edges.at(0).y<<","<<box_edges.at(1).x<<","<<box_edges.at(1).y<<","<<box_edges.at(2).x<<","<<box_edges.at(2).y<<","<<box_edges.at(3).x<<","<<box_edges.at(3).y<<","<<"\n";
                        if(siz!=0)
                        {
                            Find_detec=1;
                            detec=0;
                        }

                        else
                            detec=1;
                    }
                    if(Find_detec){
                        corners1 =Tag.Tag_Calculate_Features(gray,tag_points);
                        not_detect=false;
                    }
                    else
                    {
                        corners.clear();
                        corners0.clear();
                        not_detect=true;
                    }
                    size_c=corners1.size();
                    corners.resize(size_c);

                    corners=corners1;
                    corners0.resize(corners.size());
                    corners0=corners;
                    flag=true;
                }
                else
                    break;

                begin_detect=false;
            }
        }
    }
    Tag.Tag_Destroy(getopt,tf,td,detections);
 //   myfile_detections.close();
}

vector<Point2f> nedges;
void *trackingv(void *i)
{
    ofstream myfile_tracking;
    vector<Mat>next_edges;
    Features_Tracking Track;
    Mat H;
    vector<Point2f> corners_t,corners0_t,prev_edges;
    Mat src;
    Mat rotation,translation;
    Pose_Estimation pose;

    ofstream myfile_detections;
    myfile_detections.open("/home/imad/Desktop/evaluation/detections.csv");
    myfile_detections<<"detectx1"<<","<<"detecty1"<<","<<"detect2"<<","<<"detect2"<<","<<"detectx3"<<","<<"detecty3"<<","<<"detectx4"<<","<<"detecty4"<<","<<"\n";


    myfile_tracking.open("/home/imad/Desktop/evaluation/tracking.csv");
    myfile_tracking<<"trackx1"<<","<<"tracky1"<<","<<"trackx2"<<","<<"tracky2"<<","<<"trackx3"<<","<<"tracky3"<<","<<"trackx4"<<","<<"tracky4"<<","<<"\n";

    int n=1;
    int nf;
    int det;
    while(1)
    {
        prev_edges=box_edges;
        //  cout<<"box_edges :\n"<<box_edges<<endl;
        det=Find_detec;
        nf=0;
        while(nf!=n)
        {
            nf++;
            // cap_mutex.lock();
            cap>>src;
            // cap_mutex.unlock();
            //waitKey(30);

            if(!src.empty())
            {
                if(flag)
                {
                    corners_t.resize(size_c);
                    corners_t=corners;
                    corners0_t.resize(size_c);
                    corners0_t=corners0;
                    next_corners.resize(size_c);
                    flag=false;
                }
                cvtColor( src, src_gray, CV_BGR2GRAY );
                if(prevgray.empty())
                    src_gray.copyTo(prevgray);
                if(!not_detect)
                {
                    next_corners= Track.OpticalFlow_Homograhpy(prevgray,src_gray,corners_t,corners0_t,H);
                    nedges=Track.OpticalFlow_tracking_box(src,prevgray,src_gray,prev_edges);
                    if(nedges.size()>0)
                    {
                        myfile_tracking<<nedges.at(0).x<<","<<nedges.at(0).y<<","<<nedges.at(1).x<<","<<nedges.at(1).y<<","<<nedges.at(2).x<<","<<nedges.at(2).y<<","<<nedges.at(3).x<<","<<nedges.at(3).y<<","<<"\n";
                        myfile_detections<<prev_edges.at(0).x<<","<<prev_edges.at(0).y<<","<<prev_edges.at(1).x<<","<<prev_edges.at(1).y<<","<<prev_edges.at(2).x<<","<<prev_edges.at(2).y<<","<<prev_edges.at(3).x<<","<<prev_edges.at(3).y<<","<<"\n";

                    }
                    Track.Show_OpticalFlow(2,src,corners_t,next_corners);
                    corners_t.resize(next_corners.size());
                   // cout<<"**************************"<<endl;
                    pose.using_solvepnp(src,nedges,rotation,translation);
                   // cout<<"translation is : \n"<<translation<<endl;
                   // cout<<"rotation is : \n"<<rotation<<endl;

                    not_detect=false;
                }
                else
                {
                    corners_t.clear();
                    corners0_t.clear();
                    next_corners.clear();
                    nedges.clear();
                }
           //     cout<<"nedges are :\n"<<nedges<<endl;

                prev_edges=nedges;
                corners_t=next_corners;
                if (Find_detec)
                {
                    Track.Show_Detection(src,tag_points);
                    Find_detec=0;
                }
                cout<<"display"<<endl;
                namedWindow( "OpticalFlow", CV_WINDOW_AUTOSIZE );
                video.write(src);
                imshow( "OpticalFlow", src );
                waitKey(1);
                swap(prevgray,src_gray);
            }
            else
                break;
        }
        begin_detect=true;
    }
    myfile_tracking.close();
    myfile_detections.close();
}
