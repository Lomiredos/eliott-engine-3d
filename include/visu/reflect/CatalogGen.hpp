#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include <cassert>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <vector>


namespace ee::reflection
{
    class CatalogVisitor : public FieldVisitor
    {
    public:
        using FieldVisitor::visit;
        std::vector<nlohmann::json> stack;

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, ee::Color3 &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) override;
        void visitEntity(const char *_name, ee::EntityRef &_value, const char *_required) override;
        void separator()override;
        void space()override;
        void label([[maybe_unused]] const char *_text)override; 


        void beginGroup(const char *_name) override;
        void endGroup() override;
        void beginArray(const char *_name, std::size_t &_size) override;
        void endArray() override;
    
        bool beginElementPrototype() override;
        void endElementPrototype() override;

        
        void add(nlohmann::json _node) {stack.back()["children"].push_back(std::move(_node));}

        CatalogVisitor(){
            nlohmann::json root;
            root["children"] = nlohmann::json::array();
            stack.push_back(std::move(root));
        }
    };

    template <typename T>
    void emitComponent(nlohmann::json &_components)
    {
        T instance{};
        CatalogVisitor cv;
        Reflect<T>::visit(instance, cv);

        nlohmann::json comp;
        comp["name"] = Reflect<T>::name;
        assert(cv.stack.size() == 1);
        comp["fields"] = cv.stack[0]["children"];
        _components.push_back(comp);
    }

    void writeCatalog(const nlohmann::json &_components, const std::filesystem::path &_out);
}
