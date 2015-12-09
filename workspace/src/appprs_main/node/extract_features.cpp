#include <ros/ros.h>
// PCL specific includes
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/common/geometry.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "../circleFit/CircleFitByTaubin.cpp"
#include <ros/time.h>
#include <iostream>
#include <iomanip>
#include <pcl/features/principal_curvatures.h>
#include <pcl/features/boundary.h>
#include <vector>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Point.h>
#include <tf/transform_broadcaster.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include <stdlib.h>
#include <appprs_main/ClassifyLegs.h>
#include <std_msgs/Float32MultiArray.h>
std::ofstream myfile;
ros::ServiceClient client;
std::vector<std::vector<float>> bbox_holder;

//void publish_boxes();

int get_features(const sensor_msgs::PointCloud2ConstPtr& input,
		const int cloud_number, std::vector<float> &bbox) {
	//std::cout<<"started get_features function"<<std::endl;
	pcl::PointCloud<pcl::PointXYZ>::Ptr input_cloud(
			new pcl::PointCloud<pcl::PointXYZ>);
	pcl::fromROSMsg(*input, *input_cloud);

	int Size = (*input_cloud).size();

	reals X[Size];
	reals Y[Size];

	// std::vector<pcl::PointXYZ> all_pts=(*input_cloud).points;

	pcl::PointCloud<pcl::PointXYZ> internal_cloud = (*input_cloud);
	float meanX = 0;
	float meanY = 0;
	std::vector<int>::const_iterator pit;

	//Extract Points from Cloud for fast lookup
	float min_x = 1000;
	float min_y = 1000;
	float max_x = -1000;
	float max_y = -1000;

	for (int i = 0; i < Size; i++) {
		X[i] = internal_cloud.points[i].x;
		Y[i] = internal_cloud.points[i].y;
		if (X[i] < min_x)
			min_x = X[i];
		if (Y[i] < min_y)
			min_y = Y[i];
		if (X[i] > max_x)
			max_x = X[i];
		if (Y[i] > max_y)
			max_y = Y[i];
	}
	//Calculating Circles and stuff
	Data data1(Size, X, Y);

	Circle circle;
	circle = CircleFitByTaubin(data1);
	//	cout << "\n  Taubin fit:  center (" << circle.a << "," << circle.b
	//			<< ")  radius " << circle.r << "  sigma " << circle.s << std::endl;

	//Calculate Mean
	meanX = data1.meanX;
	meanY = data1.meanY;

	//Calcualte distances between consecutive points
	float dist_mean = 0;
	float boundary_length = 0;
	float angle_avg;
	float dist[Size - 1];
	for (int j = 0; j < Size - 1; j++) {
		float X1 = X[j];
		float X2 = X[j + 1];
		float Y1 = Y[j];
		float Y2 = Y[j + 1];

		dist[j] = sqrt(pow(X2 - X1, 2) + pow(Y2 - Y1, 2));
		boundary_length += dist[j];

		float dot = X1 * X2 + Y1 * Y2;
		float det = X1 * Y2 - Y1 * X2;
		angle_avg += atan2(det, dot);

	}
	dist_mean = boundary_length / (Size - 1);
	angle_avg = angle_avg / (Size - 1);
	//Calculate Curvature between 3 pts
	float avg_curve = 0;
	for (int i = 1; i < Size - 2; i++) {
		float Ax = X[i];
		float Ay = Y[i];
		float Bx = X[i + 1];
		float By = Y[i + 1];
		float Cx = X[i + 2];
		float Cy = Y[i + 2];

		float A = std::abs(
				((Ax * (By - Cy) + Bx * (Cy - Ay) + Cx * (Ay - By)) / 2));
		float dA = sqrt(pow(Bx - Ax, 2) + pow(By - Ay, 2));
		float dB = sqrt(pow(Cx - Bx, 2) + pow(Cy - By, 2));
		float dC = sqrt(pow(Ax - Cx, 2) + pow(Ay - Cy, 2));

		avg_curve += 4 * A / (dA * dB * dC);
	}
	avg_curve = avg_curve / (Size - 2);

	//boundary standard Dev
	float std_dist = 0;
	for (int j = 0; j < Size - 1; j++) {
		std_dist += pow(dist[j] - dist_mean, 2);
	}
	std_dist = sqrt(1.0 / ((float) Size - 1) * std_dist);

	//Calculating STD of Points away from mean
	float std_xy = 0;
	for (int i = 0; i < Size; i++) {
		std_xy += pow(X[i] - meanX, 2) + pow(Y[i] - meanY, 2);
	}
	float temp_err = std_xy;
	std_xy = sqrt(1.0 / (float) Size * temp_err);
	//std::cout<<"doing multi_array_things"<<std::endl;
	std_msgs::Float32MultiArray features;
	//float features[13];

	float SCALEME = 100000;
	features.data.push_back(Size);//[0] = Size; //number of points
	features.data.push_back(std_xy * SCALEME);//[1] = std_xy * SCALEME;  //Standard Dev from Mean
	features.data.push_back(0.5 * SCALEME);//[2] = 0.5 * SCALEME; //mean distance from median
	features.data.push_back(1 * SCALEME);//[3] = 1 * SCALEME; //jump distance from last segment (not doing)
	features.data.push_back(0.5 * SCALEME);// = 0.5 * SCALEME; //just distance from next segment (nto doing)
	features.data.push_back(sqrt(pow(X[Size - 1] - X[0], 2) + pow(Y[Size - 1] - Y[0], 2)) * SCALEME);//[5] = sqrt(
	//			pow(X[Size - 1] - X[0], 2) + pow(Y[Size - 1] - Y[0], 2)) * SCALEME; //width
	features.data.push_back(1 * SCALEME);//[3] = 1 * SCALEME; //jump distance from last segment (not doing)
	features.data.push_back((float)circle.s*SCALEME);//[7] = circle.s * SCALEME; //Sum of squared residuals
	features.data.push_back((float)circle.r*SCALEME);// = circle.r * SCALEME; //radius of circle
	features.data.push_back(boundary_length*SCALEME);// = boundary_length * SCALEME;
	features.data.push_back(std_dist*SCALEME);// = std_dist * SCALEME; //standard deviation of distances between points
	features.data.push_back(avg_curve*SCALEME);// = avg_curve * SCALEME; //mean curvature of object
	features.data.push_back(angle_avg*SCALEME);// = angle_avg * SCALEME; //mean angle between consecutive points
	//std::cout<<"multi_array_worked"<<std::endl;
	if (0) {
		myfile << ros::Time::now() << ',' << cloud_number << ',';
		for (int i = 0; i < 13; i++) {

			myfile << std::setprecision(5) << features.data[i] << ',';
		}
		myfile << std::endl;
	}
	//std::cout<<"make srv"<<std::endl;
	appprs_main::ClassifyLegs srv;

	//std::cout << "call service" << std::endl;
	srv.request.features = features;
	int lab = client.call(srv);


	//Calcluate bounding box around point
	bbox.push_back(min_x);
	bbox.push_back(max_x);
	bbox.push_back(min_y);
	bbox.push_back(max_y);
	bbox.push_back((float)lab);
	//bbox_holder.at(cloud_number)=bbox;

	//std::cout << "service called successfully" << std::endl;

	std::cout << "cloud number: " << cloud_number << "label: " << lab
			<< std::endl;
	return lab;

}

void cloud1_cb(const sensor_msgs::PointCloud2ConstPtr& input) {
	if (!(*input).data.empty()) {
		std::vector<float> bbox;
		int label = get_features(input, 2, bbox);

	}
}
void cloud2_cb(const sensor_msgs::PointCloud2ConstPtr& input) {
	if (!(*input).data.empty()) {
		std::vector<float> bbox;
		int label = get_features(input, 2, bbox);
	}
}
void cloud3_cb(const sensor_msgs::PointCloud2ConstPtr& input) {
	if (!(*input).data.empty()) {
		std::vector<float> bbox;
		int label = get_features(input, 3, bbox);
	}
}
void cloud4_cb(const sensor_msgs::PointCloud2ConstPtr& input) {
	if (!(*input).data.empty()) {
		std::vector<float> bbox;
		int label = get_features(input, 4, bbox);
	}
}
void cloud5_cb(const sensor_msgs::PointCloud2ConstPtr& input) {
	if (!(*input).data.empty()) {
		std::vector<float> bbox;
		int label = get_features(input, 5, bbox);
	}
}
void cloud6_cb(const sensor_msgs::PointCloud2ConstPtr& input) {
	if (!(*input).data.empty()) {
		std::vector<float> bbox;
		int label = get_features(input, 6, bbox);
	}
}

int main(int argc, char** argv) {
	//std::cout<<"about to initialize everything"<<std::endl;
	// Initialize ROS
	ros::init(argc, argv, "my_pcl_filter");
	ros::NodeHandle nh;
	ros::Publisher vis_pub = nh.advertise<visualization_msgs::Marker>(
			"bounding_boxes", 0);

	//std::cout<<"about to initialize service client"<<std::endl;
	ros::ServiceClient client = nh.serviceClient<
			appprs_main::ClassifyLegs>("classify_legs");

	myfile.open("features.csv");

	// Create a ROS subscriber for the input point cloud
	//std::cout<<"making subscribers"<<std::endl;
	ros::Subscriber sub1 = nh.subscribe("cluster_1", 1, cloud1_cb);
	ros::Subscriber sub2 = nh.subscribe("cluster_2", 1, cloud2_cb);
	ros::Subscriber sub3 = nh.subscribe("cluster_3", 1, cloud3_cb);
	ros::Subscriber sub4 = nh.subscribe("cluster_4", 1, cloud4_cb);
	ros::Subscriber sub5 = nh.subscribe("cluster_5", 1, cloud5_cb);
	ros::Subscriber sub6 = nh.subscribe("cluster_6", 1, cloud6_cb);
	// Spin
	ros::spin();
}

/*void publish_boxes() {
	visualization_msgs::Marker points, line_strip, line_list;
	visualization_msgs::MarkerArray marker_array;
	points.header.frame_id = line_strip.header.frame_id = line_list.header.frame_id = "/body_link";
	points.header.stamp = line_strip.header.stamp = line_list.header.stamp = ros::Time::now();
	points.ns = line_strip.ns = line_list.ns = "points_and_lines";
	points.action = line_strip.action = line_list.action = visualization_msgs::Marker::ADD;
	points.pose.orientation.w = line_strip.pose.orientation.w = line_list.pose.orientation.w = 1.0;

	points.type=visualization_msgs::Marker::POINTS;
	line_strip.type=visualization_msgs::Marker::LINE_STRIP;
	line_strip.type=visualization_msgs::Marker::LINE_LIST;

	points.id = 0;
	line_strip.id = 1;
	line_list.id = 2;

    // POINTS markers use x and y scale for width/height respectively
    points.scale.x = 0.2;
    points.scale.y = 0.2;

    // LINE_STRIP/LINE_LIST markers use only the x component of scale, for the line width
    line_strip.scale.x = 0.1;
    line_list.scale.x = 0.1;



    // Points are green
    points.color.g = 1.0f;
    points.color.a = 1.0;

    // Line strip is blue
    line_strip.color.b = 1.0;
    line_strip.color.a = 1.0;

    // Line list is red
    line_list.color.r = 1.0;
    line_list.color.a = 1.0;


	//0=xmin
	//1=xmax
	//2=ymin
	//3=ymax

for(int i=0; i<bbox_holder.size(); i++)
{
	std::vector<float> bbox(bbox_holder.at(0));
	geometry_msgs::Point plr, pll, pur, pul;
	pll.x=bbox.at(0);
	pll.y=bbox.at(2);
	pll.z=0;

	plr.x=bbox.at(1);
	plr.y=bbox.at(2);
	plr.z=0;

	pur.x=bbox.at(1);
	pur.y=bbox.at(3);
	pur.z=0;

	pul.x=bbox.at(0);
	pul.y=bbox.at(3);
	pul.z=0;
	points.points.push_back(pll);
	points.points.push_back(plr);
	points.points.push_back(pur);
	points.points.push_back(pul);
	points.points.push_back(pll);

	line_strip.points.push_back(pll);
	line_strip.points.push_back(plr);
	line_strip.points.push_back(pur);
	line_strip.points.push_back(pul);
	line_strip.points.push_back(pll);
	marker_array.markers.push_back(line_strip);

}



}*/



