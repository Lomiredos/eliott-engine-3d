#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>

// ---------------------------------------------------------------------------
// Generation du catalogue Components.json a partir des structs C++ reflechies.
//
// emitComponent<T>() instancie un T par defaut, le fait visiter par un
// CatalogVisitor (qui aplatit Vector3/Quaternion en floats -- option A), et
// pousse une entree { name, fields:[{name,type,default}] } dans le tableau.
// C'est l'equivalent "fait main" de la reflexion runtime que le C# offre a
// Unity : le C++ n'en ayant pas, on la joue au build.
// ---------------------------------------------------------------------------

namespace ee::reflection
{
    // Remplit un tableau JSON de champs {name, type, default}.
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

    // Emet l'entree catalogue du composant T. T doit specialiser Reflect<T>
    // (membres `name` + `visit`). Les defauts viennent de T{} (initialiseurs
    // de la struct).
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

    // Ecrit { "components": [...] } dans _out (JSON indente).
    void writeCatalog(const nlohmann::json &_components, const std::filesystem::path &_out);
}
