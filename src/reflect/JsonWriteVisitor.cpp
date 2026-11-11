#include "visu/reflect/JsonWriteVisitor.hpp"

#include <charconv>
#include <cstdlib>
#include <string>


double ee::reflection::shortestFloat(float _value) {
    char buf[32];
    auto res = std::to_chars(buf, buf + sizeof(buf), _value);
    if (res.ec != std::errc()) return static_cast<double>(_value);
    *res.ptr = '\0';
    return std::strtod(buf, nullptr);
}

nlohmann::ordered_json* ee::reflection::JsonWriteVisitor::slot(const char* _name) {
    if (m_stack.empty() || m_stack.back() == nullptr) return nullptr;

    nlohmann::ordered_json& cur = *m_stack.back();

    if (cur.is_array()) {
        char* end = nullptr;
        unsigned long i = std::strtoul(_name, &end, 10);
        if (end == _name || *end != '\0') return nullptr;
        if (i >= cur.size()) cur.get_ref<nlohmann::ordered_json::array_t&>().resize(i + 1);
        return &cur[i];
    }

    if (cur.is_null()) cur = nlohmann::ordered_json::object();
    if (!cur.is_object()) return nullptr;
    return &cur[_name];
}

void ee::reflection::JsonWriteVisitor::visit(const char* _name, int& _value) {
    if (nlohmann::ordered_json* j = slot(_name)) *j = _value;
}

void ee::reflection::JsonWriteVisitor::visit(const char* _name, bool& _value) {
    if (nlohmann::ordered_json* j = slot(_name)) *j = _value;
}

void ee::reflection::JsonWriteVisitor::visit(const char* _name, float& _value) {
    if (nlohmann::ordered_json* j = slot(_name)) *j = shortestFloat(_value);
}

void ee::reflection::JsonWriteVisitor::visit(const char* _name, ee::Color3& _value) {
    if (nlohmann::ordered_json* j = slot(_name)) *j = ee::toHex(_value);
}

void ee::reflection::JsonWriteVisitor::visit(const char* _name, std::string& _value) {
    if (nlohmann::ordered_json* j = slot(_name)) *j = _value;
}

void ee::reflection::JsonWriteVisitor::beginGroup(const char* _name) {
    nlohmann::ordered_json* j = slot(_name);
    if (j) *j = nlohmann::ordered_json::object();
    m_stack.push_back(j);
}

void ee::reflection::JsonWriteVisitor::endGroup() { m_stack.pop_back(); }

void ee::reflection::JsonWriteVisitor::visitEnum(const char* _name, int& _value, const char* const* _labels,
                                                 int _count) {
    nlohmann::ordered_json* j = slot(_name);
    if (!j) return;
    if (_value >= 0 && _value < _count) *j = _labels[_value];
    else                                *j = _value;
}

void ee::reflection::JsonWriteVisitor::beginArray(const char* _name, std::size_t& _size) {
    nlohmann::ordered_json* j = slot(_name);
    if (j) {
        *j = nlohmann::ordered_json::array();
        j->get_ref<nlohmann::ordered_json::array_t&>().resize(_size);
    }
    m_stack.push_back(j);
}

void ee::reflection::JsonWriteVisitor::endArray() { m_stack.pop_back(); }
