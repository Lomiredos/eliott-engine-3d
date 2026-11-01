#pragma once

#include <vector>
#include <memory>
#include <cassert>
#include <type_traits>

namespace ee::fsm {


    template <typename T> class Transition;

    template<typename T>
    class Condition 
    {
        bool Test(const T& _owner){
            return expected == OnTest(_owner);
        }

    public:
        Condition(bool _expected = true) : expected(_expected) {}
        virtual ~Condition() = default;

        bool expected = true; 

        virtual bool OnTest(const T& _owner) = 0;


        friend Transition<T>; // transition can call Test
    };


    template<typename T>
    class Transition
    {
    protected:
        std::vector<std::unique_ptr<Condition<T>>> m_conditions;
        int m_transitionState;

    public:
        Transition(int _transitionState) : m_transitionState(_transitionState) {}
        virtual ~Transition() = default;


        template<typename U>
        void AddCondition(bool _expected = true){
        static_assert(std::is_base_of<Condition<T>, U>::value, "U must be derived from Condition");

        std::unique_ptr<U> pCondition = std::make_unique<U>();

        pCondition->expected = _expected;

        m_conditions.push_back(std::move(pCondition));
        }
        
        bool Try(const T& owner){

        for (const auto& c : m_conditions) {
            if (c->Test(owner) == false)
                return false;
        }

        return true;
        }

        int GetTransitionState() { return m_transitionState; }
    };

    template <typename T> class StateMachine;

    template<typename T>
    class Action
    {
        std::vector<std::unique_ptr<Transition<T>>> m_transitions;

    private:


        int Update(T& _owner, float _dt){
        OnUpdate(_owner, _dt);
        

        //check if one transition is aviable
        for (const auto& t : m_transitions)
        {
            if (t->Try(_owner))
            {
                return t->GetTransitionState();
            }
        }

        return -1; // -1 == none
    }
        

    public:
        virtual ~Action() = default;

        Transition<T>* CreateTransition(int state){
            std::unique_ptr<Transition<T>> pTransition = std::make_unique<Transition<T>>(state);
            m_transitions.push_back(std::move(pTransition));
            return m_transitions.back().get();
        }

    protected:
        virtual void OnStart(T& _owner) = 0;
        virtual void OnUpdate(T& _owner, float _dt) = 0; // no const because it may modifie owner
        virtual void OnEnd(T& _owner) = 0;



        friend StateMachine<T>; // stateMachine can call private function of Action as update
    };



    template<typename T>
    class StateMachine
    {
        T& m_owner;
        std::vector<std::unique_ptr<Action<T>>> m_actions;
        int m_currentState;

    public:

        StateMachine(T& _owner) : m_owner(_owner), m_currentState(-1) {}

        StateMachine(const StateMachine&) = delete;      
        StateMachine& operator=(const StateMachine&) = delete;
        StateMachine(StateMachine&&) = default;

        ~StateMachine() = default; //no nead to delete unique_ptr;
        
        void Update(float _dt) {
            if (m_currentState == -1)
                return;
            int transitionState = m_actions[m_currentState]->Update(m_owner, _dt);
            
            if (transitionState == -1)
                return;
            SetState(transitionState);
        }
        
        void SetState(int _state) {
        
            assert(_state >= 0 && _state < (int)m_actions.size() && m_actions[_state] && "_state must be a valide index of state");
            if (_state < 0 || _state >= (int)m_actions.size() || !m_actions[_state])
                return;


            if (m_currentState >= 0)
                m_actions[m_currentState]->OnEnd(m_owner);
            
            m_currentState = _state;
            m_actions[m_currentState]->OnStart(m_owner);
        }


        int GetCurrentState() const { return m_currentState; }
        
        template<typename U>
        U* CreateAction(int _state)
        {
            assert(_state == (int)m_actions.size() && "_state must be striclty equal to the number of state");
            static_assert(std::is_base_of<Action<T>, U>::value, "your Action must be derived from Action<>");
            std::unique_ptr<U> pAction = std::make_unique<U>();
            U* raw = pAction.get();
            m_actions.push_back(std::move(pAction));
            return raw;
        }
    };

} //namespace ee::fsm