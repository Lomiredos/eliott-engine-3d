#pragma once

#include "math/Vector3.hpp"
#include "math/Quaternion.hpp"

#include "visu/core/Color3.hpp"
#include "visu/core/EntityRef.hpp"
#include "visu/reflect/Reflect.hpp"

#include <string>

#include <cstddef>
#include <type_traits>
#include <vector>



template <typename E>
struct EnumNames;

namespace ee::reflection
{
    class FieldVisitor
    {
    public:
        virtual ~FieldVisitor() = default;

        virtual void visit(const char *_name, int &_value) = 0;
        virtual void visit(const char *_name, bool &_value) = 0;
        virtual void visit(const char *_name, float &_value) = 0;
        virtual void visit(const char *_name, ee::Color3 &_value) = 0;
        virtual void visit(const char *_name, std::string &_value) = 0;
        virtual void visit(const char *_name, ee::math::Vector3<float> &_value) = 0;
        virtual void visit(const char *_name, ee::math::Quaternion &_value) = 0;


        virtual void visitEnum(const char *_name, int &_value, const char *const *_labels, int _count) = 0;
        virtual void visitEntity(const char *_name, ee::EntityRef &_value, const char *_required) = 0;
        virtual void beginArray(const char *_name, std::size_t &_size) = 0;
        virtual void endArray() = 0;

        void visit(const char *_name, ee::EntityRef &_value) { visitEntity(_name, _value, nullptr); }

        template <typename C>
        void visit(const char *_name, ee::EntityRefOf<C> &_value)
        {
            visitEntity(_name, _value, Reflect<C>::name);
        }

        template <typename E>
            requires std::is_enum_v<E>
        void visit(const char *_name, E &_value)
        {
            int raw = static_cast<int>(_value);
            visitEnum(_name, raw, EnumNames<E>::values, EnumNames<E>::count);
            _value = static_cast<E>(raw);
        }

        template <typename T>
        void visit(const char *_name, std::vector<T> &_value){
            std::size_t size = _value.size();
            beginArray(_name, size);
            _value.resize(size);
            for (T &elem : _value)
                visit(nullptr, elem);
            endArray();
        }
    };
}
