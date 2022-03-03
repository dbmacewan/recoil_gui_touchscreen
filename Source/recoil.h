#pragma once
#ifndef recoil_h
#define recoil_h

#include <string>
#include <vector>
#include <math.h>
#include "recoil_structs.h"


class Recoil {
public:

    Recoil(); // default constructor
    void fireBullet(int8_t& x, int8_t& y); // returns recoil compensated x/y values
    bool isActive(); // state of recoil compensation
    void setActive(); // turns recoil compensation on
    void setInactive(); // turns recoil compensation off
    void setEquipment(RECOIL_EQUIPMENT type); // change weapon/sight/barrel type
    void setSensitivity(float value); // input ingame user sens
    void setFOV(float value); // input ingame user fov
    std::string getWeapon(); // current weapon type
    std::string getBarrel(); // current barrel type
    std::string getSight(); // current sight type
    std::string getSensitivity(); // current sens
    std::string getFOV(); // current fov
    double getUpdateRate(); // current update rate in ms. mouse movement should be called in sync with this rate
    ~Recoil(); // destructor


private:

	WEAPON current; // current weapon data
	std::vector<ANGLES> lerpAngles; // final calculated values in pixels
	attachmentData barrel; // current barrel attachment data
	attachmentData sight; // current sight attachment data

	int16_t active = -1; // the state of the recoil adjustment, used to access the index of the recoil table
	uint8_t bullet = 1; // used to initialize the vector
	uint8_t updateRate = 32; // update rate in ms for injected mouse movements
	uint8_t updatePerBullet; // how many times the mouse is moved per shot fired
    RECOIL_EQUIPMENT e_weapon; // enum for weapon
    RECOIL_EQUIPMENT e_barrel; // enum for barrel
    RECOIL_EQUIPMENT e_sight; // enum for sight
	float sensitivity = 5.0; // user mouse sens
	float fov = 90.0; // user field of view

    void setWeapon(RECOIL_EQUIPMENT type); // set weapon type
    void setSight(RECOIL_EQUIPMENT type); // set sight modifier type
    void setBarrel(RECOIL_EQUIPMENT type); // set barrel modifier type
    void reset(); // resets all recoil table data
    void calcRecoilTable(); // calculates all values in recoil table
    void AngleConversion(); // converts angles to pixels
    void Lerp(); // converts total pixel movement into smaller increments - "smoothing"
};

#endif