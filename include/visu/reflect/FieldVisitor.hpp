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

    class FieldVisitor;
    template <typename T>
    concept Reflectable = requires (T &_t, FieldVisitor &_v) {
        Reflect<T>::visit(_t, _v);
    };


    class FieldVisitor
    {
    public:
        virtual ~FieldVisitor() = default;

        virtual void visit(const char *_name, int &_value) = 0;
        virtual void visit(const char *_name, bool &_value) = 0;
        virtual void visit(const char *_name, float &_value) = 0;
        virtual void visit(const char *_name, ee::Color3 &_value) = 0;
        virtual void visit(const char *_name, std::string &_value) = 0;
        virtual void beginGroup(const char *_name) = 0;
        virtual void endGroup() = 0;
        virtual void separator(){}
        virtual void space(){}
        virtual void label([[maybe_unused]] const char *_text){} 
        // [[maybe_unused]] sert a dire au compilateur -> c'est ok que j'utilise pas l'arg qui vien apres dans la fonction, et a pas se tapper de warning
        //vus qu'on l'utilise dans les fonction qui override celle ci

        void visit(const char *_name, ee::math::Vector3<float> &_value){
            beginGroup(_name);
            visit("x", _value.x);
            visit("y", _value.y);
            visit("z", _value.z);
            endGroup();
        }
        void visit(const char *_name, ee::math::Quaternion &_value){
            beginGroup(_name);
            visit("w", _value.w);
            visit("x", _value.x);
            visit("y", _value.y);
            visit("z", _value.z);
            endGroup();
        }

        template <typename S>
            requires Reflectable<S>
        void visit(const char *_name, S &_value);


        virtual void visitEnum(const char *_name, int &_value, const char *const *_labels, int _count) = 0;
        virtual void visitEntity(const char *_name, ee::EntityRef &_value, const char *_required) = 0;
        virtual void beginArray(const char *_name, std::size_t &_size) = 0;
        virtual void endArray() = 0;

        virtual bool beginElementPrototype() {return false;}
        virtual void endElementPrototype(){}


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
            static_assert(EnumNames<E>::count > 0, "EnumName : met une valeur dans l'enum, sinon c'est pas toptop" );
            static_assert(EnumNames<E>::count == std::size(EnumNames<E>::values), "EnumName : count est different du nombre de valeur possible, comment t'a fait ?" );
            int raw = static_cast<int>(_value);
            visitEnum(_name, raw, EnumNames<E>::values, EnumNames<E>::count);
            _value = static_cast<E>(raw);
        }

        template <typename T>
        void visit(const char *_name, std::vector<T> &_value){
            static_assert(!std::is_same_v<T, bool>, "std::vector<bool> non supporter, utilise des char ou des int plutot stp");
            std::size_t size = _value.size();
            beginArray(_name, size);
            _value.resize(size);
            for (size_t i = 0; i < size; i++){
                const std::string n = std::to_string(i);
                visit(n.c_str(), _value[i]);
            }
            if (beginElementPrototype()){
                T proto{};
                visit("element", proto);
                endElementPrototype();
            }
            endArray();
        }
    };


    struct GroupScope{
        [[nodiscard]] GroupScope(FieldVisitor &_v, const char *_name) : v(_v) {v.beginGroup(_name);}
        ~GroupScope() {v.endGroup();}
        GroupScope(const GroupScope &) = delete;
        GroupScope &operator=(const GroupScope &) = delete;

        private:
        FieldVisitor &v;

    };

    template <typename S>
        requires Reflectable<S>
    void FieldVisitor::visit(const char *_name, S &_value)
    {
        GroupScope g(*this, _name);
        Reflect<S>::visit(_value, *this);
    }
    
}
