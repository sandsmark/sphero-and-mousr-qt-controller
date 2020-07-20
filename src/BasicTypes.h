#pragma once

#pragma pack(push, 1)

template<typename T>
struct Vector3D {
    T x;
    T y;
    T z;
};

template<typename T>
struct Vector2D {
    T x;
    T y;
};

template<typename T>
struct Orientation {
    T pitch;
    T roll;
    T yaw;
};

template<typename T>
struct Quaternion {
    T scalar;
    T x;
    T y;
    T z;
};

#pragma pack(pop)


inline float rssiToStrength(const int rssi)
{
    if (rssi == 0) { // special case in QtBluetooth apparently
        return 0.f;
    }
    if(rssi <= -100) {
        return 0.f;
    } else if(rssi >= -50) {
        return 1.f;
    } else {
        return 2.f * (rssi/100.f + 1.f);
    }
}
