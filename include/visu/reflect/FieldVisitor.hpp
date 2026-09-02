#pragma once

#include "math/Vector3.hpp"
#include "math/Quaternion.hpp"

#include <string>


namespace ee::reflection
{
    class FieldVisitor
    {
    public:
        virtual ~FieldVisitor() = default;

        virtual void visit(const char *_name, int &_value) = 0;
        virtual void visit(const char *_name, bool &_value) = 0;
        virtual void visit(const char *_name, float &_value) = 0;
        virtual void visit(const char *_name, std::string &_value) = 0;
        virtual void visit(const char *_name, ee::math::Vector3<float> &_value) = 0;
        virtual void visit(const char *_name, ee::math::Quaternion &_value) = 0;
    };
}
