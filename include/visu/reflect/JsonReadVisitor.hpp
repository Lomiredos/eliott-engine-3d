#pragma once

#include <nlohmann/json.hpp>

#include "nlohmann/json_fwd.hpp"
#include "visu/core/Color3.hpp"
#include "visu/reflect/FieldVisitor.hpp"

namespace ee::reflection {

    class JsonReadVisitor : public ee::reflection::FieldVisitor {
        using FieldVisitor::visit;
        std::vector<const nlohmann::json*> m_stack;

        const nlohmann::json* find(const char* _name) const;


    public:
        void visit(const char* _name, int& _value) override;
        void visit(const char* _name, bool& _value) override;
        void visit(const char* _name, float& _value) override;
        void visit(const char* _name, ee::Color3& _value) override;
        void visit(const char* _name, std::string& _value) override;

        void beginGroup(const char* _name) override;
        void endGroup() override;

        void visitEnum(const char* _name, int& _value, const char* const* _labels, int _count) override;
        void visitEntity(const char* _name, ee::EntityRef& _value, const char* _required) override {}

        void beginArray(const char* _name, std::size_t& _size) override;
        void endArray() override;

        JsonReadVisitor(const nlohmann::json* j) {
            m_stack.push_back(j);
        }
    };
}