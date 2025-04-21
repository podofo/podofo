/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PODOFO_BASE_TYPES_H
#define PODOFO_BASE_TYPES_H
#pragma once

#include <string>
#include <memory>
#include "span.h"

namespace PoDoFo
{
    /** Convenient read-only char buffer span
     */
    using bufferview = cspan<char>;

    /** Convenient writable char buffer span
     */
    using bufferspan = mspan<char>;

    /** Unicode code point view
     */
    using unicodeview = cspan<char32_t>;

    // TODO: Optimize, maintaining string compatibility
    // Use basic_string::resize_and_overwrite in C++23
    // https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
    /**
     * Convenient type for char array storage and/or buffer with
     * std::string compatibility
     */
    template <typename = void>
    class charbuff_t final : public std::string
    {
    public:
        charbuff_t() noexcept { }
        charbuff_t(const charbuff_t&) = default;
        charbuff_t(charbuff_t&&) noexcept = default;
        charbuff_t(size_t size)
        {
            std::string::resize(size);
        }
        charbuff_t(std::string&& str)
            : std::string(std::move(str)) { }
        charbuff_t(const char* str, size_t size)
            : std::string(str, size) { }
        explicit charbuff_t(const bufferview& view)
            : std::string(view.data(), view.size()) { }
        explicit charbuff_t(const std::string_view& view)
            : std::string(view) { }
        explicit charbuff_t(const std::string& str)
            : std::string(str) { }
    public:
        charbuff_t& operator=(const charbuff_t&) = default;
        charbuff_t& operator=(charbuff_t&&) noexcept = default;
        charbuff_t& operator=(const std::string_view & view)
        {
            std::string::assign(view.data(), view.size());
            return *this;
        }
        charbuff_t& operator=(const std::string& str)
        {
            std::string::assign(str.data(), str.size());
            return *this;
        }
        charbuff_t& operator=(const bufferview& view)
        {
            std::string::assign(view.data(), view.size());
            return *this;
        }
        charbuff_t& operator=(std::string&& str) noexcept
        {
            std::string::operator=(std::move(str));
            return *this;
        }
        bool operator==(const charbuff_t& rhs) const
        {
            return static_cast<const std::string&>(*this) == static_cast<const std::string&>(rhs);
        }
    private:
        size_t length() const = delete;
    };

    template <typename = void>
    bool operator==(const charbuff_t<>& lhs, const char* rhs)
    {
        return static_cast<const std::string&>(lhs) == rhs;
    }

    template <typename = void>
    bool operator==(const char* lhs, const charbuff_t<>& rhs)
    {
        return lhs == static_cast<const std::string&>(rhs);
    }

    template <typename = void>
    bool operator==(const charbuff_t<>& lhs, const bufferview& rhs)
    {
        return static_cast<const std::string&>(lhs) == std::string_view(rhs.data(), rhs.size());
    }

    template <typename = void>
    bool operator==(const bufferview& lhs, const charbuff_t<>& rhs)
    {
        return std::string_view(lhs.data(), lhs.size()) == static_cast<const std::string&>(rhs);
    }

    template <typename = void>
    bool operator==(const charbuff_t<>& lhs, const std::string_view& rhs) noexcept
    {
        return std::string_view(lhs) == rhs;
    }

    template <typename = void>
    bool operator==(const std::string_view& lhs, const charbuff_t<>& rhs) noexcept
    {
        return lhs == std::string_view(rhs);
    }

    template <typename = void>
    bool operator==(const charbuff_t<>& lhs, const std::string& rhs) noexcept
    {
        return static_cast<const std::string&>(lhs) == rhs;
    }

    template <typename = void>
    bool operator==(const std::string& lhs, const charbuff_t<>& rhs) noexcept
    {
        return lhs == static_cast<const std::string&>(rhs);
    }

    using charbuff = charbuff_t<>;

    /** A const data provider that can hold a view to a
     * static segments or a shared buffer
     *
     */
    template <typename = void>
    class datahandle_t final
    {
    public:
        datahandle_t() { }
        datahandle_t(const bufferview& view)
            : m_view(view) { }
        datahandle_t(charbuff_t<>&& buff)
            : m_buff(new charbuff_t<>(std::move(buff))), m_view(*m_buff) { }
        datahandle_t(std::unique_ptr<const charbuff_t<>>&& buff)
            : m_buff(std::move(buff)), m_view(*m_buff) { }
        datahandle_t(std::unique_ptr<charbuff_t<>>&& buff)
            : m_buff(std::move(buff)), m_view(*m_buff) { }
        datahandle_t(std::shared_ptr<const charbuff_t<>> buff)
            : m_buff(std::move(buff)), m_view(*m_buff) { }
        datahandle_t(std::shared_ptr<charbuff_t<>> buff)
            : m_buff(std::move(buff)), m_view(*m_buff) { }
    public:
        const bufferview& view() const { return m_view; }
    private:
        std::shared_ptr<const charbuff_t<>> m_buff;
        bufferview m_view;
    };

    using datahandle = datahandle_t<>;

    // https://artificial-mind.net/blog/2020/10/03/always-false
    template <class... T>
    constexpr bool always_false = false;
}

#endif // PODOFO_BASE_TYPES_H
