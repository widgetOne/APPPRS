/*
 * singleparticle.h
 *
 *  Created on: Nov 16, 2015
 *      Author: jamie
 */

#ifndef SINGLEPARTICLE_H_
#define SINGLEPARTICLE_H_

class single_particle {
public:
	//Default Constructor
	single_particle();
	//Default Destructor
	virtual ~single_particle();

	//Overload Constructor
	single_particle(int x, int y, int th);

	//Accessor Functions
	float getX() const;
	float getY() const;
	float getTh() const;
	float getWeight() const;

	//Mutator Functions
	void setX(float);
	void setY(float);
	void setTh(float);
	void setPosition(float var_x, float var_y, float var_Th);
	void setWeight(float var_weight);

	//Output Functions
	float evaluateLidar(float lidar[180]);
	void evaluateOdometry(float dx, float dy, float dtheta, float dtime);




private:
	float x;
	float y;
	float theta;
	float weight;

};




#endif /* SINGLEPARTICLE_H_ */