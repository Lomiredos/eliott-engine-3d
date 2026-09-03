#include "visu/reflect/CatalogGen.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace ee::reflection
{
    namespace
    {
        nlohmann::json makeField(const std::string &_name, const char *_type, nlohmann::json _def)
        {
            nlohmann::json f;
            f["name"] = _name;
            f["type"] = _type;
            f["default"] = std::move(_def);
            return f;
        }
    }

    void CatalogVisitor::visit(const char *_name, int &_value)
    {
        fields.push_back(makeField(_name, "int", _value));
    }
    void CatalogVisitor::visit(const char *_name, bool &_value)
    {
        fields.push_back(makeField(_name, "bool", _value));
    }
    void CatalogVisitor::visit(const char *_name, float &_value)
    {
        fields.push_back(makeField(_name, "float", _value));
    }
    void CatalogVisitor::visit(const char *_name, std::string &_value)
    {
        fields.push_back(makeField(_name, "string", _value));
    }

    void CatalogVisitor::visit(const char *_name, ee::math::Vector3<float> &_value)
    {
        std::string s(_name);
        fields.push_back(makeField(s + "X", "float", _value.x));
        fields.push_back(makeField(s + "Y", "float", _value.y));
        fields.push_back(makeField(s + "Z", "float", _value.z));
    }
    void CatalogVisitor::visit(const char *_name, ee::math::Quaternion &_value)
    {
        std::string s(_name);
        fields.push_back(makeField(s + "W", "float", _value.w));
        fields.push_back(makeField(s + "X", "float", _value.x));
        fields.push_back(makeField(s + "Y", "float", _value.y));
        fields.push_back(makeField(s + "Z", "float", _value.z));
    }

    void CatalogVisitor::visitEnum(const char *_name, int& _value, const char* const* _labels, int _count)
    {
        nlohmann::json options = nlohmann::json::array();
        for (int i = 0; i < _count; i++){
            options.push_back(_labels[i]);
        }
        nlohmann::json f = makeField(_name, "enum", _labels[_value]);
        f["options"] = options;
        field.push_back(f);
    } 


    void writeCatalog(const nlohmann::json &_components, const std::filesystem::path &_out)
    {
        nlohmann::json root;
        root["components"] = _components;

        std::ofstream f(_out, std::ios::binary);
        f << root.dump(2) << "\n";

        std::cout << "[gen] catalogue ecrit : " << _out.string()
                  << " (" << _components.size() << " composant(s))\n";
    }
}
