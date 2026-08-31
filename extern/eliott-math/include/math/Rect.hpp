#pragma once

#include "Vector2.hpp"

namespace ee::math
{

    template <typename T>

    class Rect
    {
    private:
        Vector2<T> m_position;
        Vector2<T> m_size;

    public:
        Rect() = default;

        // init avec des vec
        Rect(Vector2<T> _pos, Vector2<T> _size) : m_position(_pos), m_size(_size) {}

        // init avec des pos
        Rect(T _x, T _y, T _w, T _h) : m_position(Vector2<T>(_x, _y)), m_size(Vector2<T>(_w, _h)) {}

        Vector2<T> getPosition(float _anchorX = 0.5f, float _anchorY = 0.5f) const
        {
            return m_position + Vector2<T>(static_cast<T>(m_size.x * _anchorX), static_cast<T>(m_size.y * _anchorY));
        }

        Vector2<T> getSize() const
        {
            return m_size;
        }

        void setSize(Vector2<T> _size)
        {
            m_size = _size;
        }

        void setPosition(Vector2<T> _position, float _anchorX = 0.5f, float _anchorY = 0.5f)
        {
            m_position = _position - Vector2<T>(static_cast<T>(m_size.x * _anchorX), static_cast<T>(m_size.y * _anchorY));
        }

        bool isInside(Vector2<T> _point) const
        {
            bool isXok = _point.x > m_position.x && _point.x < m_position.x + m_size.x;
            bool isYok = _point.y > m_position.y && _point.y < m_position.y + m_size.y;
            return (isXok && isYok);
        }

        bool isInside(T _x, T _y) const
        {
            bool isXok = _x > m_position.x && _x < m_position.x + m_size.x;
            bool isYok = _y > m_position.y && _y < m_position.y + m_size.y;
            return (isXok && isYok);
        }

        bool Intersects(const Rect<T> &_other) const
        {

            bool XCollide = !(_other.getPosition(1).x < m_position.x || _other.getPosition(0).x > getPosition(1).x);

            bool YCollide = !(_other.getPosition(1, 1).y < m_position.y || _other.getPosition(0, 0).y > getPosition(1, 1).y);

            return (XCollide && YCollide);
        }

        bool contains(const Rect<T> &_other) const
        {

            bool topLeft = isInside(_other.getPosition(0, 0));
            bool bottomRight = isInside(_other.getPosition(1, 1));

            return (topLeft && bottomRight);
        }

        template <typename U>
        explicit operator Rect<U>() const
        {
            return Rect<U>(static_cast<Vector2<U>>(m_position), static_cast<Vector2<U>>(m_size));
        }
    };

}