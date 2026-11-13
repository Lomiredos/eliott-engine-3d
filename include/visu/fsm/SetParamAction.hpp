#pragma once
#include "visu/fsm/API.hpp"
#include "visu/fsm/State.hpp"

#include <string>
#include <variant>

#include "visu/fsm/Params.hpp"
#include "visu/fsm/Utils.hpp"

namespace ee::fsm {

    class SetParamAction : public ee::fsm::Action {

    private:
        std::string m_param;
        ee::fsm::ParamValue m_value;

    public:
        virtual void OnStart(ee::fsm::Ctx& _ctx) override {
            int index = ee::fsm::findParam(_ctx.def, m_param);
            if (index < 0) return;
            switch (_ctx.def.params[index].type) {
                case ee::fsm::ParamType::Bool: {
                    if (const bool* v = std::get_if<bool>(&m_value)) ee::fsm::setBool(_ctx.def, _ctx.inst, index, *v);
                    break;
                }

                case ee::fsm::ParamType::Int: {
                    if (const int* v = std::get_if<int>(&m_value)) ee::fsm::setInt(_ctx.def, _ctx.inst, index, *v);
                    break;
                }

                case ee::fsm::ParamType::Float: {

                    if (const float* v = std::get_if<float>(&m_value))
                        ee::fsm::setFloat(_ctx.def, _ctx.inst, index, *v);
                    break;
                }

                case ee::fsm::ParamType::Trigger: {
                    const bool* v = std::get_if<bool>(&m_value);
                    if (v && !*v) ee::fsm::resetTrigger(_ctx.def, _ctx.inst, index);
                    else          ee::fsm::setTrigger(_ctx.def, _ctx.inst, index);
                    break;
                }
            }
        }

        virtual void VisitParams(ee::reflection::FieldVisitor& _v) override {
            _v.visit("param", m_param);
            _v.visit("value", m_value);
        }
    };
} // namespace ee::fsm