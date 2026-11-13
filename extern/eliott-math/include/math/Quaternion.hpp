#pragma once
#include <cmath>
#include "Vector3.hpp"

namespace ee::math
{

    struct Quaternion
    {
        float w = 1.f, x = 0.f, y = 0.f, z = 0.f;

        static Quaternion identity() { return {1.f, 0.f, 0.f, 0.f}; }

        static Quaternion fromAxisAngle(Vector3<float> axis, float angleRad)
        {
            float half = angleRad * 0.5f;
            float s = sinf(half);
            return {cosf(half), axis.x * s, axis.y * s, axis.z * s};
        }

        // Rotation depuis des angles d'Euler en DEGRES, appliques dans l'ordre
        // X, puis Y, puis Z (q = qz * qy * qx : la mult. applique qx en 1er).
        static Quaternion fromEulerDeg(float rxDeg, float ryDeg, float rzDeg)
        {
            const float k = 3.14159265358979323846f / 180.0f;
            Quaternion qx = fromAxisAngle({1, 0, 0}, rxDeg * k);
            Quaternion qy = fromAxisAngle({0, 1, 0}, ryDeg * k);
            Quaternion qz = fromAxisAngle({0, 0, 1}, rzDeg * k);
            return qz * qy * qx;
        }
        static Quaternion fromEulerDeg(Vector3<float> _rDeg) {
            const float k = 3.14159265358979323846f / 180.0f;
            Quaternion qx = fromAxisAngle({1, 0, 0}, _rDeg.x * k);
            Quaternion qy = fromAxisAngle({0, 1, 0}, _rDeg.y * k);
            Quaternion qz = fromAxisAngle({0, 0, 1}, _rDeg.z * k);
            return qz * qy * qx;
        }

        Vector3<float> toEulerDeg() const {

            const double r00 = 1.0 - 2.0 * (y * y + z * z);
            const double r01 = 2.0 * (x * y - w * z);
            const double r02 = 2.0 * (x * z + w * y);
            const double r10 = 2.0 * (x * y + w * z);
            const double r20 = 2.0 * (x * z - w * y);
            const double r21 = 2.0 * (y * z + w * x);
            const double r22 = 1.0 - 2.0 * (x * x + y * y);

            double ax, ay, az;
            if (r20 <= -0.999999) // Y = +90
            {
                ay = 3.14159265358979323846 / 2.0;
                az = 0.0;
                ax = std::atan2(r01, r02);
            } else if (r20 >= 0.999999) // Y = -90
            {
                ay = -3.14159265358979323846 / 2.0;
                az = 0.0;
                ax = std::atan2(-r01, -r02);
            } else {
                ay = std::asin(-r20);
                ax = std::atan2(r21, r22);
                az = std::atan2(r10, r00);
            }
            const double toDeg = 180.0 / 3.14159265358979323846;
            return {static_cast<float>(ax * toDeg), static_cast<float>(ay * toDeg), static_cast<float>(az * toDeg)};
        }

        Quaternion operator*(const Quaternion &q) const
        {
            return {
                w * q.w - x * q.x - y * q.y - z * q.z,
                w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y - x * q.z + y * q.w + z * q.x,
                w * q.z + x * q.y - y * q.x + z * q.w};
        }

        Quaternion Normalize() const
        {
            float len = sqrtf(w * w + x * x + y * y + z * z);
            if (len > 0.f)
                return {w / len, x / len, y / len, z / len};
            return identity();
        }

        Vector3<float> rotate(Vector3<float> v) const
        {
            Quaternion qv = {0.f, v.x, v.y, v.z};
            Quaternion conj = {w, -x, -y, -z};
            Quaternion res = (*this * qv) * conj;
            return {res.x, res.y, res.z};
        }

        Vector3<float> forward() const { return rotate({0, 0, -1}); }
        Vector3<float> back() const { return rotate({0, 0, 1}); }
        Vector3<float> up() const { return rotate({0, 1, 0}); }
        Vector3<float> right() const { return rotate({1, 0, 0}); }
    };

}
