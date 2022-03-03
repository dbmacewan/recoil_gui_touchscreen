#include <math.h>
#include "recoil.h"

/*****************     PUBLIC    *******************/


Recoil::Recoil() {
    active = -1;
    bullet = 1;
    updateRate = 16;
    sensitivity = 5.0;
    fov = 90.0;
    current = none;
    updatePerBullet = 1;
    barrel = noMult;
    sight = noMult;
    e_weapon = RECOIL_EQUIPMENT::WEAPON_NONE;
    e_barrel = RECOIL_EQUIPMENT::BARREL_NONE;
    e_sight = RECOIL_EQUIPMENT::SIGHT_NONE;
    calcRecoilTable();
}

void Recoil::fireBullet(int8_t& x, int8_t& y) {
    if (active < static_cast<int>(lerpAngles.size())) {
        x = static_cast<int>(lerpAngles[active].x);
        y = static_cast<int>(lerpAngles[active].y);
        active++;
        return;
    }
    else {
        x = 0;
        y = 0;
        return;
    }
}

bool Recoil::isActive() {
    if (active < 0 || active >= static_cast<int>(lerpAngles.size())) {
        return false;
    }
    return true;
}

void Recoil::setActive() {
    if (active == -1) {
        active = 0;
    }
}

void Recoil::setInactive() {
    active = -1;
    bullet = 1;
}


void Recoil::setEquipment(RECOIL_EQUIPMENT type) {
    if (type <= RECOIL_EQUIPMENT::WEAPON_SAR) {
        reset();
        setWeapon(type);
        setSight(e_sight);
        setBarrel(e_barrel);
    }
    else if (type == RECOIL_EQUIPMENT::BARREL_NONE || type == RECOIL_EQUIPMENT::BARREL_SUPPR) {
        reset();
        setBarrel(type);
        setSight(e_sight);
        setWeapon(e_weapon);
    }
    else if (type >= RECOIL_EQUIPMENT::SIGHT_NONE && type <= RECOIL_EQUIPMENT::SIGHT_SIMPLE) {
        reset();
        setSight(type);
        setBarrel(e_barrel);
        setWeapon(e_weapon);
    }
    else {
        reset();
    }
    calcRecoilTable();
}

void Recoil::setSensitivity(float value) {
    if (value <= 0) {
        return;
    }
    else {
        sensitivity = value;
        reset();
        setWeapon(e_weapon);
        setSight(e_sight);
        setBarrel(e_barrel);
        calcRecoilTable();
    }
}

void Recoil::setFOV(float value) {
    if (value < 75 || value > 120) {
        return;
    }
    else {
        fov = value;
        reset();
        setWeapon(e_weapon);
        setSight(e_sight);
        setBarrel(e_barrel);
        calcRecoilTable();
    }
}

std::string Recoil::getWeapon() {
    return current.type;
}

std::string Recoil::getBarrel() {
    return barrel.name;
}

std::string Recoil::getSight() {
    return sight.name;
}

std::string Recoil::getSensitivity() {
    if (sensitivity <= 0) {
        return "0";
    }
    std::string result;
    int8_t numPrecision = 5;
    int decimalPlaceIndex;
    int tempInt = static_cast<int>(sensitivity);
    int tempFrac = (sensitivity - tempInt) * pow(10, numPrecision); // extract 5 digits

    result = static_cast<char>((tempInt % 10) + 48);
    tempInt /= 10;
    while (tempInt > 0) {
        result.insert(0, 1, static_cast<char>((tempInt % 10) + 48));
        tempInt /= 10;
    }
    result += ".";
    decimalPlaceIndex = result.size();

    // remove leading 0's
    while (tempFrac % 10 == 0) {
        tempFrac /= 10;
        numPrecision--;
    }

    // if no fractional part exists append a 0 and return
    if (!tempFrac) {
        result.append("0");
        return result;
    }

    while (numPrecision > 0) {
        result.insert(decimalPlaceIndex, 1, static_cast<char>((tempFrac % 10) + 48));
        tempFrac /= 10;
        numPrecision--;
    }

    return result;
}

std::string Recoil::getFOV() {
    uint16_t temp = static_cast<uint16_t>(fov);
    uint8_t d0, d1, d2;
    std::string result;
    if (temp < 100) {
        d0 = temp % 10;
        temp /= 10;
        d1 = temp % 10;
        result = static_cast<char>(d1 + 48);
        result += static_cast<char>(d0 + 48);
    }
    else {
        d0 = temp % 10;
        temp /= 10;
        d1 = temp % 10;
        temp /= 10;
        d2 = temp % 10;
        result = static_cast<char>(d2 + 48);
        result += static_cast<char>(d1 + 48);
        result += static_cast<char>(d0 + 48);
    }

    return result;
}

double Recoil::getUpdateRate() {
    return current.repeatDelay / updatePerBullet;
}

Recoil::~Recoil() {
}


/**********************    PRIVATE    ************************/

void Recoil::setWeapon(RECOIL_EQUIPMENT type) {
    switch (type) {
    case RECOIL_EQUIPMENT::WEAPON_NONE:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_NONE;
        current = none;
        break;

    case RECOIL_EQUIPMENT::WEAPON_AK:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_AK;
        current = assualt_rifle;
        break;

    case RECOIL_EQUIPMENT::WEAPON_LR:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_LR;
        current = lr_assault_rifle;
        break;

    case RECOIL_EQUIPMENT::WEAPON_MP5:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_MP5;
        current = mp5;
        break;

    case RECOIL_EQUIPMENT::WEAPON_TOMMY:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_TOMMY;
        current = thompson;
        break;

    case RECOIL_EQUIPMENT::WEAPON_CUST:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_CUST;
        current = custom;
        break;

    case RECOIL_EQUIPMENT::WEAPON_M249:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_M249;
        current = m249;
        break;

    case RECOIL_EQUIPMENT::WEAPON_M39:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_M39;
        current = m39;
        break;

    case RECOIL_EQUIPMENT::WEAPON_M92:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_M92;
        current = m92;
        break;

    case RECOIL_EQUIPMENT::WEAPON_PYTHON:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_PYTHON;
        current = python;
        break;

    case RECOIL_EQUIPMENT::WEAPON_REVVY:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_REVVY;
        current = revolver;
        break;

    case RECOIL_EQUIPMENT::WEAPON_P2:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_P2;
        current = semiPistol;
        break;

    case RECOIL_EQUIPMENT::WEAPON_SAR:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_SAR;
        current = sar;
        break;

    default:
        e_weapon = RECOIL_EQUIPMENT::WEAPON_NONE;
        current = none;
        break;
    }
}

void Recoil::setSight(RECOIL_EQUIPMENT type) {
    switch (type) {
    case RECOIL_EQUIPMENT::SIGHT_NONE:
        e_sight = RECOIL_EQUIPMENT::SIGHT_NONE;
        sight = noMult;
        break;

    case RECOIL_EQUIPMENT::SIGHT_HOLO:
        e_sight = RECOIL_EQUIPMENT::SIGHT_HOLO;
        sight = holo;
        break;

    case RECOIL_EQUIPMENT::SIGHT_8X:
        e_sight = RECOIL_EQUIPMENT::SIGHT_8X;
        sight = scope8;
        break;

    case RECOIL_EQUIPMENT::SIGHT_SIMPLE:
        e_sight = RECOIL_EQUIPMENT::SIGHT_SIMPLE;
        sight = simple;
        break;

    default:
        e_sight = RECOIL_EQUIPMENT::SIGHT_NONE;
        sight = noMult;
        break;
    }
}

void Recoil::setBarrel(RECOIL_EQUIPMENT type) {
    switch (type) {
    case RECOIL_EQUIPMENT::BARREL_NONE:
        e_barrel = RECOIL_EQUIPMENT::BARREL_NONE;
        barrel = noMult;
        break;

    case RECOIL_EQUIPMENT::BARREL_SUPPR:
        e_barrel = RECOIL_EQUIPMENT::BARREL_SUPPR;
        barrel = suppressor;
        break;

    default:
        e_barrel = RECOIL_EQUIPMENT::BARREL_NONE;
        barrel = noMult;
        break;
    }
}

void Recoil::reset() {
    lerpAngles.clear();
    current.angles.clear();
    setInactive();
    updatePerBullet = 1;
}

void Recoil::calcRecoilTable() {
    if (current.type == "none") {
        updatePerBullet = 1;
    }
    else {
        updatePerBullet = static_cast<uint8_t>(current.repeatDelay) / updateRate;
    }

    while (bullet < current.angles.size()) {
        AngleConversion();
        Lerp();
        bullet++;
    }
    bullet = 1;
}

void Recoil::AngleConversion() {
    current.angles[bullet].x = current.angles[bullet].x / (-0.03 * static_cast<double>(sensitivity) * 3.0 * (fov / 100.0));
    current.angles[bullet].y = current.angles[bullet].y / (-0.03 * static_cast<double>(sensitivity) * 3.0 * (fov / 100.0));
    if (e_weapon == RECOIL_EQUIPMENT::WEAPON_TOMMY || e_weapon == RECOIL_EQUIPMENT::WEAPON_CUST) {
        current.angles[bullet].x *= 0.847;
        current.angles[bullet].y *= 0.847;
    }
    current.angles[bullet].x *= barrel.mult;
    current.angles[bullet].y *= barrel.mult;
    current.angles[bullet].x *= sight.mult;
    current.angles[bullet].y *= sight.mult;
}

void Recoil::Lerp() {
    for (uint8_t i = 0; i < updatePerBullet; i++) {
        ANGLES temp;
        temp.x = round(current.angles[bullet].x / updatePerBullet);
        temp.y = round(current.angles[bullet].y / updatePerBullet);
        lerpAngles.push_back(temp);
    }
}