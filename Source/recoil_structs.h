#pragma once
#ifndef RECOIL_STRUCTS_H
#define RECOIL_STRUCTS_H
#include <string>
#include <vector>

enum class RECOIL_EQUIPMENT {
	WEAPON_NONE,// 0
	WEAPON_AK,/// 1
	WEAPON_LR,// 2
	WEAPON_MP5,// 3
	WEAPON_TOMMY,// 4
	WEAPON_CUST,// 5
	WEAPON_M249,// 6
	WEAPON_M39,// 7
	WEAPON_M92,// 8
	WEAPON_PYTHON,// 9
	WEAPON_REVVY,// 10
	WEAPON_P2,// 11
	WEAPON_SAR,// 12
	BARREL_NONE,// 13
	BARREL_SUPPR,// 14
	SIGHT_NONE,// 15
	SIGHT_HOLO,// 16
	SIGHT_8X,// 17
	SIGHT_SIMPLE,// 18
};


struct ANGLES {
	double x;
	double y;
};

struct WEAPON {
	std::string type; // name of weapon
	std::vector<ANGLES> angles; // weapons x and y position
	double repeatDelay; // weapons repeat delay
};

struct attachmentData {
	std::string name;
	double mult;
};


extern WEAPON assualt_rifle;
extern WEAPON lr_assault_rifle;
extern WEAPON mp5;
extern WEAPON thompson;
extern WEAPON custom;
extern WEAPON m249;
extern WEAPON m39;
extern WEAPON m92;
extern WEAPON python;
extern WEAPON revolver;
extern WEAPON semiPistol;
extern WEAPON sar;
extern WEAPON none;

extern attachmentData noMult;
extern attachmentData scope8;
extern attachmentData holo;
extern attachmentData simple;
extern attachmentData suppressor;

#endif