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
