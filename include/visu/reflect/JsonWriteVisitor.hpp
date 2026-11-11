#pragma once

#include <nlohmann/json.hpp>

#include <vector>

#include "visu/core/Color3.hpp"
#include "visu/reflect/FieldVisitor.hpp"

namespace ee::reflection {

    class JsonWriteVisitor : public ee::reflection::FieldVisitor {
        using FieldVisitor::visit;
        std::vector<nlohmann::ordered_json*> m_stack;

        nlohmann::ordered_json* slot(const char* _name);

    public:
        void visit(const char* _name, int& _value) override;
        void visit(const char* _name, bool& _value) override;
        void visit(const char* _name, float& _value) override;
        void visit(const char* _name, ee::Color3& _value) override;
        void visit(const char* _name, std::string& _value) override;

        void beginGroup(const char* _name) override;
        void endGroup() override;

        void visitEnum(const char* _name, int& _value, const char* const* _labels, int _count) override;
        void visitEntity(const char*, ee::EntityRef&, const char*) override {}

        void beginArray(const char* _name, std::size_t& _size) override;
        void endArray() override;

        explicit JsonWriteVisitor(nlohmann::ordered_json* _j) {
            m_stack.push_back(_j);
        }
    };

    double shortestFloat(float _value);
}
