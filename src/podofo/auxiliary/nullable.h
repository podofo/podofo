/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef AUX_NULLABLE_H
#define AUX_NULLABLE_H
#pragma once

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace PoDoFo
{
    class bad_nullable_access : public std::runtime_error
    {
    public:
        bad_nullable_access()
            : std::runtime_error("nullable object doesn't have a value") { }
    };

    /**
     * Alternative to std::optional that supports reference (but not pointer) types
     */
    template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    class nullable final
    {
    public:
        nullable()
            : m_dummy{ }, m_hasValue(false) { }

        nullable(T value)
            : m_value(std::move(value)), m_hasValue(true) { }

        nullable(std::nullptr_t)
            : m_dummy{ }, m_hasValue(false) { }

        nullable(const nullable& value)
            : m_dummy{ }
        {
            if (value.m_hasValue)
            {
                new(&m_value)T(value.m_value);
                m_hasValue = true;
            }
            else
            {
                m_hasValue = false;
            }
        }

        ~nullable()
        {
            if (m_hasValue)
                m_value.~T();
        }

        nullable& operator=(const nullable& value)
        {
            if (m_hasValue)
            {
                if (value.m_hasValue)
                {
                    m_value = value.m_value;
                }
                else
                {
                    m_value.~T();
                    m_hasValue = false;
                }
            }
            else
            {
                if (value.m_hasValue)
                {
                    new(&m_value)T(value.m_value);
                    m_hasValue = true;
                }
            }

            return *this;
        }

        nullable& operator=(T value)
        {
            if (m_hasValue)
            {
                m_value = std::move(value);
            }
            else
            {
                new(&m_value)T(std::move(value));
                m_hasValue = true;
            }

            return *this;
        }

        nullable& operator=(std::nullptr_t)
        {
            if (m_hasValue)
                m_value.~T();

            m_hasValue = false;
            return *this;
        }

        operator nullable<const T&>() const
        {
            if (m_hasValue)
                return nullable<const T&>(m_value);
            else
                return { };
        }

        const T& value() const
        {
            if (!m_hasValue)
                throw bad_nullable_access();

            return m_value;
        }

        bool has_value() const { return m_hasValue; }
        const T* operator->() const { return &m_value; }
        const T& operator*() const { return m_value; }
        operator const T* () const
        {
            if (m_hasValue)
                return &m_value;
            else
                return nullptr;
        }

    public:
        template <typename T2>
        friend bool operator==(const nullable<T2>& lhs, const nullable<T2>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& lhs, const nullable<T2>& rhs);

        template <typename T2>
        friend bool operator==(const nullable<std::decay_t<T2>>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<std::decay_t<T2>>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator==(const nullable<T2&>& lhs, const nullable<std::decay_t<T2>>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<T2&>& lhs, const nullable<std::decay_t<T2>>& rhs);

        template <typename T2>
        friend bool operator==(const nullable<T2>& lhs, const T2& rhs);

        template <typename T2>
        friend bool operator==(const T2& lhs, const nullable<T2>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& lhs, const T2& rhs);

        template <typename T2>
        friend bool operator!=(const T2& lhs, const nullable<T2>& rhs);

        template <typename T2>
        friend std::enable_if_t<!std::is_reference_v<T2>, bool> operator==(const nullable<T2>& lhs, std::nullptr_t);

        template <typename T2>
        friend std::enable_if_t<!std::is_reference_v<T2>, bool> operator==(std::nullptr_t, const nullable<T2>& rhs);

        template <typename T2>
        friend std::enable_if_t<!std::is_reference_v<T2>, bool> operator!=(const nullable<T2>& lhs, std::nullptr_t);

        template <typename T2>
        friend std::enable_if_t<!std::is_reference_v<T2>, bool> operator!=(std::nullptr_t, const nullable<T2>& rhs);

    private:
        struct NonTrivialDummyType
        {
            constexpr NonTrivialDummyType() noexcept
            {
                // Avoid zero-initialization when objects are value-initialized
                // Inspired from MS STL https://github.com/microsoft/STL/blob/8124540f8bce3faad76a6dddd050f9a69af4b87d/stl/inc/optional#L58
            }
        };

        union
        {
            NonTrivialDummyType m_dummy;
            T m_value;
        };
        bool m_hasValue;
    };

    // Template specialization for references
    template <typename TRef>
    class nullable<TRef, std::enable_if_t<std::is_reference_v<TRef>>> final
    {
        using T = std::remove_reference_t<TRef>;
    public:
        nullable()
            : m_value{ } { }

        nullable(T& value)
            : m_value(&value) { }

        nullable(T* value)
            : m_value(value) { }

        nullable(std::nullptr_t)
            : m_value{ } { }

        // Allow nullable<const T&>::nullable(const nullable<T&>&)
        template <typename T2, typename = std::enable_if_t<
            std::is_convertible_v<std::add_pointer_t<std::remove_reference_t<T2>>,
                std::add_pointer_t<std::remove_reference_t<T>>>, int>>
        nullable(const nullable<T2&>& value)
            : m_value(reinterpret_cast<const nullable&>(value).m_value) { }

        nullable(const nullable& value) = default;

        nullable& operator=(const nullable& value) = default;

        T& value()
        {
            if (m_value == nullptr)
                throw bad_nullable_access();

            return *m_value;
        }

        bool has_value() const { return m_value != nullptr; }

        explicit operator T* () const { return m_value; }
        T* operator->() const { return m_value; }
        T& operator*() const { return *m_value; }

    public:
        template <typename T2>
        friend bool operator==(const nullable<std::decay_t<T2>>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator==(const nullable<T2&>& lhs, const nullable<std::decay_t<T2>>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<std::decay_t<T2>>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<T2&>& lhs, const nullable<std::decay_t<T2>>& rhs);

        template <typename T2>
        friend bool operator==(const nullable<T2&>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator==(const nullable<T2&>& lhs, const std::decay_t<T2>& rhs);

        template <typename T2>
        friend bool operator==(const std::decay_t<T2>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<T2&>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<T2&>& lhs, const std::decay_t<T2>& rhs);

        template <typename T2>
        friend bool operator!=(const std::decay_t<T2>& lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend std::enable_if_t<std::is_reference_v<T2>, bool> operator==(const nullable<T2>& lhs, std::nullptr_t);

        template <typename T2>
        friend std::enable_if_t<std::is_reference_v<T2>, bool> operator==(std::nullptr_t, const nullable<T2>& rhs);

        template <typename T2>
        friend std::enable_if_t<std::is_reference_v<T2>, bool> operator!=(const nullable<T2>& lhs, std::nullptr_t);

        template <typename T2>
        friend std::enable_if_t<std::is_reference_v<T2>, bool> operator!=(std::nullptr_t, const nullable<T2>& rhs);

        template <typename T2>
        friend bool operator==(const nullable<T2&>& lhs, const T2* rhs);

        template <typename T2>
        friend bool operator==(const T2* lhs, const nullable<T2&>& rhs);

        template <typename T2>
        friend bool operator!=(const nullable<T2&>& lhs, const T2* rhs);

        template <typename T2>
        friend bool operator!=(const T2* lhs, const nullable<T2&>& rhs);

    private:
        T* m_value;
    };

    template <typename T2>
    bool operator==(const nullable<T2>& lhs, const nullable<T2>& rhs)
    {
        if (lhs.m_hasValue != rhs.m_hasValue)
            return false;

        if (lhs.m_hasValue)
            return lhs.m_value == rhs.m_value;
        else
            return true;
    }

    template <typename T2>
    bool operator!=(const nullable<T2>& lhs, const nullable<T2>& rhs)
    {
        if (lhs.m_hasValue != rhs.m_hasValue)
            return true;

        if (lhs.m_hasValue)
            return lhs.m_value != rhs.m_value;
        else
            return false;
    }

    template <typename T2>
    bool operator==(const nullable<std::decay_t<T2>>& lhs, const nullable<T2&>& rhs)
    {
        if (lhs.m_hasValue != rhs.has_value())
            return false;

        if (lhs.m_hasValue)
            return lhs.m_value == *rhs.m_value;
        else
            return true;
    }

    template <typename T2>
    bool operator!=(const nullable<std::decay_t<T2>>& lhs, const nullable<T2&>& rhs)
    {
        if (lhs.m_hasValue != rhs.has_value())
            return true;

        if (lhs.m_hasValue)
            return lhs.m_value != *rhs.m_value;
        else
            return false;
    }

    template <typename T2>
    bool operator==(const nullable<T2&>& lhs, const nullable<std::decay_t<T2>>& rhs)
    {
        if (lhs.has_value() != rhs.m_hasValue)
            return false;

        if (lhs.has_value())
            return *lhs.m_value == rhs.m_value;
        else
            return true;
    }

    template <typename T2>
    bool operator!=(const nullable<T2&>& lhs, const nullable<std::decay_t<T2>>& rhs)
    {
        if (lhs.has_value() != rhs.m_hasValue)
            return true;

        if (lhs.has_value())
            return *lhs.m_value != rhs.m_value;
        else
            return false;
    }

    template <typename T2>
    bool operator==(const nullable<T2&>& lhs, const nullable<T2&>& rhs)
    {
        if (lhs.has_value() != rhs.has_value())
            return false;

        if (lhs.has_value())
            return *lhs.m_value == *rhs.m_value;
        else
            return true;
    }

    template <typename T2>
    bool operator!=(const nullable<T2&>& lhs, const nullable<T2&>& rhs)
    {
        if (lhs.has_value() != rhs.has_value())
            return true;

        if (lhs.has_value())
            return *lhs.m_value != *rhs.m_value;
        else
            return false;
    }

    template <typename T2>
    bool operator==(const nullable<T2&>& lhs, const std::decay_t<T2>& rhs)
    {
        if (!lhs.has_value())
            return false;

        return *lhs.m_value == rhs;
    }

    template <typename T2>
    bool operator!=(const nullable<T2&>& lhs, const std::decay_t<T2>& rhs)
    {
        if (!lhs.has_value())
            return true;

        return *lhs.m_value != rhs;
    }

    template <typename T2>
    bool operator==(const std::decay_t<T2>& lhs, const nullable<T2&>& rhs)
    {
        if (!rhs.has_value())
            return false;

        return lhs == *rhs.m_value;
    }

    template <typename T2>
    bool operator!=(const std::decay_t<T2>& lhs, const nullable<T2&>& rhs)
    {
        if (!rhs.has_value())
            return true;

        return lhs != *rhs.m_value;
    }

    template <typename T2>
    bool operator==(const nullable<T2>& lhs, const T2& rhs)
    {
        if (!lhs.m_hasValue)
            return false;

        return lhs.m_value == rhs;
    }

    template <typename T2>
    bool operator!=(const nullable<T2>& lhs, const T2& rhs)
    {
        if (!lhs.m_hasValue)
            return true;

        return lhs.m_value != rhs;
    }

    template <typename T2>
    bool operator==(const T2& lhs, const nullable<T2>& rhs)
    {
        if (!rhs.m_hasValue)
            return false;

        return lhs == rhs.m_value;
    }

    template <typename T2>
    bool operator!=(const T2& lhs, const nullable<T2>& rhs)
    {
        if (!rhs.m_hasValue)
            return true;

        return lhs != rhs.m_value;
    }

    template <typename T2>
    std::enable_if_t<!std::is_reference_v<T2>, bool> operator==(const nullable<T2>& lhs, std::nullptr_t)
    {
        return !lhs.m_hasValue;
    }

    template <typename T2>
    std::enable_if_t<!std::is_reference_v<T2>, bool> operator!=(const nullable<T2>& lhs, std::nullptr_t)
    {
        return lhs.m_hasValue;
    }

    template <typename T2>
    std::enable_if_t<!std::is_reference_v<T2>, bool> operator==(std::nullptr_t, const nullable<T2>& rhs)
    {
        return !rhs.m_hasValue;
    }

    template <typename T2>
    std::enable_if_t<!std::is_reference_v<T2>, bool> operator!=(std::nullptr_t, const nullable<T2>& rhs)
    {
        return rhs.m_hasValue;
    }

    template <typename T2>
    std::enable_if_t<std::is_reference_v<T2>, bool> operator==(const nullable<T2>& lhs, std::nullptr_t)
    {
        return lhs.m_value == nullptr;
    }

    template <typename T2>
    std::enable_if_t<std::is_reference_v<T2>, bool> operator==(std::nullptr_t, const nullable<T2>& rhs)
    {
        return rhs.m_value == nullptr;
    }

    template <typename T2>
    std::enable_if_t<std::is_reference_v<T2>, bool> operator!=(const nullable<T2>& lhs, std::nullptr_t)
    {
        return lhs.m_value != nullptr;
    }

    template <typename T2>
    std::enable_if_t<std::is_reference_v<T2>, bool> operator!=(std::nullptr_t, const nullable<T2>& rhs)
    {
        return rhs.m_value != nullptr;
    }

    template<typename T2>
    bool operator==(const nullable<T2&>& lhs, const T2* rhs)
    {
        return lhs.m_value == rhs;
    }

    template<typename T2>
    bool operator==(const T2* lhs, const nullable<T2&>& rhs)
    {
        return lhs == rhs.m_value;
    }

    template<typename T2>
    bool operator!=(const nullable<T2&>& lhs, const T2* rhs)
    {
        return lhs.m_value != rhs;
    }

    template<typename T2>
    bool operator!=(const T2* lhs, const nullable<T2&>& rhs)
    {
        return lhs != rhs.m_value;
    }
}

#endif // AUX_NULLABLE_H
