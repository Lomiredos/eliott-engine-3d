#include "visu/reflect/JsonReadVisitor.hpp"
#include "nlohmann/json_fwd.hpp"


const nlohmann::json* ee::reflection::JsonReadVisitor::find(const char* _name) const {
    if (m_stack.empty() || m_stack.back() == nullptr) return nullptr;

    const nlohmann::json& cur = *m_stack.back();

    if (cur.is_array()) {
        char* end = nullptr;
        unsigned long i = std::strtoul(_name, &end, 10);
        if (end == _name || *end != '\0' || i >= cur.size()) return nullptr;
            return &cur[i];
    }

    if (!cur.is_object()) return nullptr;

    auto it = cur.find(_name);
    return it == cur.end() ? nullptr : &*it;
}

void ee::reflection::JsonReadVisitor::visit(const char* _name, int& _value) {
    const nlohmann::json* j = find(_name);
    if (j && j->is_number_integer()) _value = j->get<int>();
}

void ee::reflection::JsonReadVisitor::visit(const char* _name, bool& _value) {
    const nlohmann::json* j = find(_name);
    if (j && j->is_boolean()) _value = j->get<bool>();
}

void ee::reflection::JsonReadVisitor::visit(const char* _name, float& _value) {
    const nlohmann::json* j = find(_name);
    if (j && j->is_number()) _value = j->get<float>();
}

void ee::reflection::JsonReadVisitor::visit(const char* _name, ee::Color3& _value) {
    const nlohmann::json* j = find(_name);
    if (j && j->is_string()) ee::fromHex(j->get<std::string>(), _value);
}

void ee::reflection::JsonReadVisitor::visit(const char* _name, std::string& _value) {
    const nlohmann::json* j = find(_name);
    if (j && j->is_string()) _value = j->get<std::string>();
}

void ee::reflection::JsonReadVisitor::beginGroup(const char* _name) {
    const nlohmann::json* j = find(_name);
    m_stack.push_back(j && j->is_object() ? j : nullptr);
}

void ee::reflection::JsonReadVisitor::endGroup() { m_stack.pop_back(); }

void ee::reflection::JsonReadVisitor::visitEnum(const char* _name, int& _value, const char* const* _labels,
                                                int _count) {
    const nlohmann::json* j = find(_name);
    if (!j) return;

    if (j->is_string()) {
        const std::string& s = j->get_ref<const std::string&>();
        for (int i = 0; i < _count; ++i)
            if (s == _labels[i]) {
                _value = i;
                return;
            }
        return;
    }

    if (j->is_number_integer()) {
        int i = j->get<int>();
        if (i >= 0 && i < _count) _value = i;
    }
}

void ee::reflection::JsonReadVisitor::beginArray(const char* _name, std::size_t& _size) {
    const nlohmann::json* j = find(_name);
    if (j && j->is_array()) {
        _size = j->size();
        m_stack.push_back(j);
    } else {
        m_stack.push_back(nullptr);
    }
}

void ee::reflection::JsonReadVisitor::endArray() { m_stack.pop_back(); }
