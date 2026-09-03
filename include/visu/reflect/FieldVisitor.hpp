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
        virtual void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) = 0;





        template <typename E>
        void visit(const char* _name E& _value){
            int raw = static_cast<int>(_value);
            visitEnum(_name, raw, EnumName<E>::values, EnumName<E>::count);
            _value = static_cast<E>(raw);
        }
    };
}
