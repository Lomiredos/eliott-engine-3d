#include "visu/reflect/CatalogGen.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

namespace ee::reflection
{
    namespace
    {
        nlohmann::json makeField(const std::string &_name, const char *_type, nlohmann::json _def)
        {
            nlohmann::json f;
            f["kind"] = "field";
            f["name"] = _name;
            f["type"] = _type;
            f["default"] = std::move(_def);
            return f;
        }

    }

    void CatalogVisitor::visit(const char *_name, int &_value)
    {
        add(std::move(makeField(_name, "int", _value)));
    }
    void CatalogVisitor::visit(const char *_name, bool &_value)
    {
        add(std::move(makeField(_name, "bool", _value)));
    }
    void CatalogVisitor::visit(const char *_name, float &_value)
    {
        add(std::move(makeField(_name, "float", _value)));
    }
    void CatalogVisitor::visit(const char *_name, std::string &_value)
    {
        add(std::move(makeField(_name, "string", _value)));
    }

    void CatalogVisitor::visitEnum(const char *_name, int &_value, const char *const *_labels, int _count)
    {
        nlohmann::json options = nlohmann::json::array();
        for (int i = 0; i < _count; i++)
        {
            options.push_back(_labels[i]);
        }
        const char *def = (_value >= 0 && _value < _count) ? _labels[_value] : _labels[0];
        nlohmann::json f = makeField(_name, "enum", def);
        f["options"] = options;
        add(std::move(f));
    }

    void CatalogVisitor::visitEntity(const char *_name, ee::EntityRef &, const char *_required)
    {
        nlohmann::json f = makeField(_name, "entity", -1);
        if (_required)
            f["requires"] = _required;
        add(std::move(f));
    }

    void CatalogVisitor::visit(const char *_name, ee::Color3 &_value)
    {
        add(std::move(makeField(_name, "color", ee::toHex(_value))));
    }

    void CatalogVisitor::separator(){
        nlohmann::json f;
        f["kind"] = "separator";
        add(std::move(f));
    }

    void CatalogVisitor::space(){
        nlohmann::json f;
        f["kind"] = "space";
        add(std::move(f));
        
    }
    
    
    void CatalogVisitor::label(const char *_text){
        nlohmann::json f;
        f["kind"] = "label";
        f["text"] = _text;
        add(std::move(f));
        
    }
    
    void CatalogVisitor::beginGroup(const char *_name){
        nlohmann::json f;
        f["kind"] = "group";
        f["name"] = _name;
        f["children"] = nlohmann::json::array();
        stack.push_back(std::move(f));
    }
    
    void CatalogVisitor::endGroup(){
        nlohmann::json done = std::move(stack.back());
        stack.pop_back();
        add(std::move(done));
    }

    void CatalogVisitor::beginArray(const char *_name, std::size_t &_size){
        nlohmann::json f;
        f["kind"] = "group";
        f["array"] = true;
        f["name"] = _name;
        f["children"] = nlohmann::json::array();
        stack.push_back(std::move(f));
    }

    void CatalogVisitor::endArray(){
        endGroup();
    }

    bool CatalogVisitor::beginElementPrototype(){
        nlohmann::json box;
        box["children"] = nlohmann::json::array();
        stack.push_back(std::move(box));
        return true;
    }

    void CatalogVisitor::endElementPrototype(){
        nlohmann::json box = std::move(stack.back());
        stack.pop_back();
        stack.back()["element"] = box["children"][0];
    }

    void writeCatalog(const nlohmann::json &_components, const std::filesystem::path &_out)
    {
        nlohmann::json root;
        root["components"] = _components;

        std::error_code ec;
        if (_out.has_parent_path())
            std::filesystem::create_directories(_out.parent_path(), ec);

        std::ofstream f(_out, std::ios::binary);
        if (!f)
        {
            std::cout << "[gen] ecriture impossible : " << _out.string() << "\n";
            return;
        }
        f << root.dump(2) << "\n";

        std::cout << "[gen] catalogue ecrit : " << _out.string()
                  << " (" << _components.size() << " composant(s))\n";
    }
}
