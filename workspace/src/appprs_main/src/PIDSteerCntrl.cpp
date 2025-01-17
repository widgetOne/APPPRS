//PID controller

#include <Eigen/Dense>
//#include "ros/ros.h"
#include "std_msgs/Float32.h"
#include "appprs_main/GoalPositionUpdater.h"

//#include "tf/transform_listener.h"
tf::TransformListener tf_listener_;


int main(int argc, char **argv)
{


  ros::init(argc, argv, "carCommand");//start node
  ros::NodeHandle n;//create handle
  ros::Publisher chatter_pub = n.advertise<std_msgs::Float32>("carCommand", 1000);
  
  tf::TransformListener tf_listener_;


  float XRob_w, YRob_w, ThRob_w, Xway_w, Yway_w, Thway_w;
  float a, b, c, d, e,f, xnew, ynew;
  float xway_r, yway_r, Th_steer_r;
  float Vel_r;
  ros::Rate loop_rate(50);


while (ros::ok())
{

  std_msgs::Float32 cmd_msg; //Create the message
  tf::StampedTransform transform;
  tf_listener_.lookupTransform("/base_link", "/map", ros::Time(0), transform);
  
  //We can do angle= tfScalar tf::Vector3::angle(transform, waypoint);

  //XRob_w=transform.getOrigin().getX;
 // YRob_w=transform.getOrigin().getX;
//  ThRob_w=transform.getRotation().getZ();

XRob_w=0;
  YRob_w=0;
  ThRob_w=0;


  //Somewhere here you get the correct waypoint coordinates in the worldframe and put them into Xway_w, Yway_w, Thway_w
  Xway_w=0;
  Yway_w=0;
  Thway_w=0;

  a=cos(ThRob_w*M_PI/180);
  b=-sin(ThRob_w*M_PI/180);
  c=XRob_w;
  d=sin(ThRob_w*M_PI/180);
  e=cos(ThRob_w*M_PI/180);
  f=YRob_w;


  xway_r=a*Xway_w+b*Yway_w+(-a*c-d*f)*1;
  yway_r=b*Xway_w+e*Yway_w+(-b*e-c*f)*1;


  if(xnew) {
    Th_steer_r=atan(ynew/xnew)*180/M_PI;
  }
  else {
    Th_steer_r=0;
  }

  if(abs(xnew)<.1)
  {
    Th_steer_r=0; 
  }

  Vel_r=.25;


  ROS_INFO("New Steer Angle is %f, New Velocity is %f", Th_steer_r, Vel_r);


  cmd_msg.data=Th_steer_r;
  chatter_pub.publish(cmd_msg); //publish message
 

  ros::spinOnce();//Spin me right round
  loop_rate.sleep(); //Sleep to hit goal refresh rate
}

  return 0;
}
