#pragma once
#include <cmath>

namespace ee::math
{
    template <typename T>
    class Vector2
    {
    public:
        T x;
        T y;

        Vector2() : x(0), y(0) {}
        Vector2(T _x, T _y) : x(_x), y(_y) {}

        void Set(T _x, T _y)
        {
            x = _x;
            y = _y;
        }

        Vector2<T> &operator+=(const Vector2<T> &_other)
        {
            x += _other.x;
            y += _other.y;
            return *this;
        }
        Vector2<T> &operator-=(const Vector2<T> &_other)
        {
            x -= _other.x;
            y -= _other.y;
            return *this;
        }
        Vector2<T> &operator*=(const Vector2<T> &_other)
        {
            x *= _other.x;
            y *= _other.y;
            return *this;
        }
        Vector2<T> &operator/=(const Vector2<T> &_other)
        {
            if (_other.x != 0 && _other.y != 0)
            {
                x /= _other.x;
                y /= _other.y;
            }
            return *this;
        }

        Vector2<T> operator+(const Vector2<T> &_other) const
        {
            return Vector2<T>(x + _other.x, y + _other.y);
        }
        Vector2<T> operator-(const Vector2<T> &_other) const
        {
            return Vector2<T>(x - _other.x, y - _other.y);
        }
        Vector2<T> operator*(const Vector2<T> &_other) const
        {
            return Vector2<T>(x * _other.x, y * _other.y);
        }
        Vector2<T> operator*(const T _other) const
        {
            return Vector2<T>(x * _other, y * _other);
        }
        Vector2<T> operator/(const Vector2<T> &_other) const
        {
            if (_other.x != 0 && _other.y != 0)
                return Vector2<T>(x / _other.x, y / _other.y);
            return *this;
        }
        Vector2<T> operator/(const T _other) const
        {
            if (_other != 0)
                return Vector2<T>(x / _other, y / _other);
            return *this;
        }

        bool operator==(const Vector2<T> &_other) const
        {
            return x == _other.x && y == _other.y;
        }
        bool operator!=(const Vector2<T> &_other) const
        {
            return !(*this == _other);
        }

        T Magnetude() const { return std::sqrt(x * x + y * y); }
        Vector2<T> Normalize() const
        {
            if (Magnetude() > 0)
                return *this / Magnetude();
            return Vector2<T>();
        };
        T Distance(const Vector2<T> &_other) const
        {
            T dx = x - _other.x;
            T dy = y - _other.y;

            return std::sqrt(dx * dx + dy * dy);
        };

        T Dot(const Vector2<T>& _other) const {
            return x * _other.x + y * _other.y;
        }

        template <typename B>
        explicit operator B() const
        {
            return B(x, y);
        }
    };

}