/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef AUX_STATE_STACK_H
#define AUX_STATE_STACK_H

#include <stack>
#include <stdexcept>

namespace PoDoFo
{
    template <typename StateT>
    class StateStack final
    {
        struct Accessor final
        {
            friend class StateStack;
        private:
            Accessor() : m_state(nullptr) { }
            void Set(StateT& state) { m_state = &state; }
        public:
            StateT* operator->() { return m_state; }
            const StateT* operator->() const { return m_state; }
            StateT& operator*() { return *m_state; }
            const StateT& operator*() const { return *m_state; }
        private:
            StateT* m_state;
        };

    public:
        Accessor Current;
    public:
        StateStack();
        void Push();
        bool PopLenient(unsigned popCount = 1);
        void Pop(unsigned popCount = 1);
        void Clear();
        unsigned GetSize() const;
    private:
        void push(const StateT& state);
    private:
        std::stack<StateT> m_states;
    };

    template <typename StateT>
    StateStack<StateT>::StateStack()
    {
        push({ });
    }

    template <typename StateT>
    void StateStack<StateT>::Push()
    {
        push(m_states.top());
    }

    template <typename StateT>
    bool StateStack<StateT>::PopLenient(unsigned popCount)
    {
        if (popCount >= m_states.size())
            return false;

        for (unsigned i = 0; i < popCount; i++)
            m_states.pop();

        Current.Set(m_states.top());
        return true;
    }

    template<typename StateT>
    inline void StateStack<StateT>::Pop(unsigned popCount)
    {
        if (popCount >= m_states.size())
            throw std::runtime_error("Can't pop out all the states in the stack");

        for (unsigned i = 0; i < popCount; i++)
            m_states.pop();

        Current.Set(m_states.top());
    }

    template <typename StateT>
    void StateStack<StateT>::Clear()
    {
        for (unsigned i = 0; i < m_states.size(); i++)
            m_states.pop();

        push({ });
    }

    template <typename StateT>
    unsigned StateStack<StateT>::GetSize() const
    {
        return (unsigned)m_states.size();
    }

    template <typename StateT>
    void StateStack<StateT>::push(const StateT& state)
    {
        m_states.push(state);
        Current.Set(m_states.top());
    }
}

#endif // AUX_STATE_STACK_H
