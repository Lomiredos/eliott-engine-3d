#pragma once
#include <cmath>

namespace ee::math
{
    template <typename T>
    class Vector3
    {
    public:
        T x;
        T y;
        T z;

        Vector3() : x(0), y(0), z(0) {}
        Vector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

        // base
        void Set(T _x, T _y, T _z)
        {
            x = _x;
            y = _y;
            z = _z;
        }

        Vector3<T> &operator+=(const Vector3<T> &_other)
        {
            x += _other.x;
            y += _other.y;
            z += _other.z;
            return *this;
        }
        Vector3<T> &operator-=(const Vector3<T> &_other)
        {
            x -= _other.x;
            y -= _other.y;
            z -= _other.z;
            return *this;
        }
        Vector3<T> &operator*=(const Vector3<T> &_other)
        {
            x *= _other.x;
            y *= _other.y;
            z *= _other.z;
            return *this;
        }
        Vector3<T> &operator/=(const Vector3<T> &_other)
        {
            if (_other.x != 0 && _other.y != 0 && _other.z != 0)
            {
                x /= _other.x;
                y /= _other.y;
                z /= _other.z;
            }
            return *this;
        }

        Vector3<T> &operator/=(T _other)
        {
            if (_other != 0)
            {
                x /= _other;
                y /= _other;
                z /= _other;
            }
            return *this;
        }

        Vector3<T> operator+(const Vector3<T> &_other) const
        {
            return Vector3<T>(x + _other.x, y + _other.y, z + _other.z);
        }
        Vector3<T> operator-(const Vector3<T> &_other) const
        {
            return Vector3<T>(x - _other.x, y - _other.y, z - _other.z);
        }
        Vector3<T> operator-() const { return {-x, -y, -z}; }

        Vector3<T> operator*(const Vector3<T> &_other) const
        {
            return Vector3<T>(x * _other.x, y * _other.y, z * _other.z);
        }
        Vector3<T> operator*(const T _other) const
        {
            return Vector3<T>(x * _other, y * _other, z * _other);
        }
        Vector3<T> operator/(const Vector3<T> &_other) const
        {
            if (_other.x != 0 && _other.y != 0 && _other.z != 0)
                return Vector3<T>(x / _other.x, y / _other.y, z / _other.z);
            return *this;
        }
        Vector3<T> operator/(const T _other) const
        {
            if (_other != 0)
                return Vector3<T>(x / _other, y / _other, z / _other);
            return *this;
        }

        bool operator==(const Vector3<T> &_other) const
        {
            return x == _other.x && y == _other.y && z == _other.z;
        }
        bool operator!=(const Vector3<T> &_other) const
        {
            return !(*this == _other);
        }

        // utilitaire
        T Magnetude() const { return std::sqrt(x * x + y * y + z * z); }

        Vector3<T> Normalize() const
        {
            if (Magnetude() > 0)
                return *this / Magnetude();
            return Vector3<T>();
        };

        void Normalized()
        {
            if (Magnetude() > 0)
                *this /= Magnetude();
        }

        T DistanceSquared(const Vector3<T> &_other) const
        {
            T dx = x - _other.x;
            T dy = y - _other.y;
            T dz = z - _other.z;

            return dx * dx + dy * dy + dz * dz;
        }

        T Distance(const Vector3<T> &_other) const
        {
            return std::sqrt(DistanceSquared(_other));
        };

        T Dot(const Vector3<T> &_other) const
        {
            return x * _other.x + y * _other.y + z * _other.z;
        }

        Vector3<T> Cross(const Vector3<T> &_other) const
        {
            Vector3<T> crossed;
            crossed.x = y * _other.z - z * _other.y;
            crossed.y = z * _other.x - x * _other.z;
            crossed.z = x * _other.y - y * _other.x;
            return crossed;
        }

        Vector3<T> Lerp(const Vector3<T> &_other, float _t) const
        {
            Vector3<T> lerped;
            lerped.x = x * (1 - _t) + _t * _other.x;
            lerped.y = y * (1 - _t) + _t * _other.y;
            lerped.z = z * (1 - _t) + _t * _other.z;

            return lerped;
        }

        // converstion
        template <typename B>
        explicit operator B() const
        {
            return B(x, y);
        }
    };

}