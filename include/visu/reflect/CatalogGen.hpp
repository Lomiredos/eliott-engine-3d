#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>


namespace ee::reflection
{
    class CatalogVisitor : public FieldVisitor
    {
    public:
        nlohmann::json fields = nlohmann::json::array();

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visit(const char *_name, ee::math::Vector3<float> &_value) override;
        void visit(const char *_name, ee::math::Quaternion &_value) override;
    };

    template <typename T>
    void emitComponent(nlohmann::json &_components)
    {
        T instance{};
        CatalogVisitor cv;
        Reflect<T>::visit(instance, cv);

        nlohmann::json comp;
        comp["name"] = Reflect<T>::name;
        comp["fields"] = cv.fields;
        _components.push_back(comp);
    }

    void writeCatalog(const nlohmann::json &_components, const std::filesystem::path &_out);
}
