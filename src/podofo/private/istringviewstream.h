#ifndef INPUT_STRING_VIEW_STREAM_H
#define INPUT_STRING_VIEW_STREAM_H
#pragma once

/**
 * MIT License
 * 
 * Copyright (c) 2021 Francesco Pretto
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <istream>
#include <streambuf>
#include <string>
#if __cplusplus >= 201703L
#include <string_view>
#endif // __cplusplus >= 201703L

namespace cmn
{
    // Based on https://stackoverflow.com/a/31597630/213871, catlan
    template <typename CharT>
    class stringviewbuffer : public std::basic_streambuf<CharT>
    {
    public:
        using traits_type = std::char_traits<CharT>;
        using int_type = typename traits_type::int_type;
        using pos_type = typename traits_type::pos_type;
        using off_type = typename traits_type::off_type;

    public:
        stringviewbuffer()
            : m_begin(nullptr), m_end(nullptr), m_current(nullptr)
        {
        }
        stringviewbuffer(const stringviewbuffer&) = default;
        stringviewbuffer& operator=(const stringviewbuffer&) = default;
        stringviewbuffer(const CharT *buffer, size_t size)
            : m_begin(buffer), m_end(buffer + size), m_current(buffer)
        {
        }
#if __cplusplus >= 201703L
        stringviewbuffer(const std::basic_string_view<CharT> &view)
            : m_begin(view.data()), m_end(view.data() + view.size()), m_current(view.data())
        {
        }
#else
        stringviewbuffer(const std::basic_string<CharT>& str)
            : m_begin(str.data()), m_end(str.data() + str.size()), m_current(str.data())
        {
        }
#endif // __cplusplus >= 201703L

    public:
        void str(const CharT* buffer, size_t size)
        {
            std::basic_streambuf<CharT>::setg(nullptr, nullptr, nullptr);
            std::basic_streambuf<CharT>::setp(nullptr, nullptr);
            m_begin = buffer;
            m_end = buffer + size;
            m_current = buffer;
        }
#if __cplusplus >= 201703L
        void str(const std::basic_string_view<CharT>& view)
        {
            str(view.data(), view.size());
        }
#else
        void str(const std::basic_string<CharT>& str)
        {
            str(view.data(), view.size());
        }
#endif // __cplusplus >= 201703L

    protected:
        int_type underflow() override
        {
            if (m_current == m_end)
                return traits_type::eof();

            return traits_type::to_int_type(*m_current);
        }
        
        int_type uflow() override
        {
            if (m_current == m_end)
                return traits_type::eof();

            return traits_type::to_int_type(*m_current++);
        }

        int_type pbackfail(int_type ch) override
        {
            // Allows to reinsert only characters already existing
            if (m_current == m_begin || (ch != traits_type::eof() && ch != m_current[-1]))
                return traits_type::eof();

            return traits_type::to_int_type(*--m_current);
        }

        pos_type seekpos(std::streampos sp, std::ios_base::openmode which) override
        {
            (void)which;
            const CharT * current = m_begin + (std::streamoff)sp;
            if (current < m_begin || current > m_end)
                return pos_type(off_type(-1));

            m_current = current;
            return m_current - m_begin;
        }

        std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way,
            std::ios_base::openmode which) override
        {
            (void)which;
            const CharT * current = m_current;
            if (way == std::ios_base::beg)
            {
                current = m_begin + off;
            }
            else if (way == std::ios_base::cur)
            {
                current += off;
            }
            else if (way == std::ios_base::end)
            {
                current = m_end + off;
            }

            if (current < m_begin || current > m_end)
                return pos_type(off_type(-1));

            m_current = current;
            return m_current - m_begin;
        }

        std::streamsize showmanyc() override
        {
            // invariant(std::less_equal<const CharT *>()(m_current, m_end))
            return m_end - m_current;
        }

    private:
        const CharT * m_begin;
        const CharT * m_end;
        const CharT * m_current;
    };

    template <typename CharT>
    class basic_istringviewstream : public std::basic_istream<CharT>
    {
    public:
        basic_istringviewstream()
            : std::basic_istream<CharT>(nullptr)
        {
            this->rdbuf(&m_buf);
        }
        basic_istringviewstream(const CharT *buffer, size_t size)
            : std::basic_istream<CharT>(nullptr), m_buf(buffer, size)
        {
            this->rdbuf(&m_buf);
        }
#if __cplusplus >= 201703L
        basic_istringviewstream(const std::basic_string_view<CharT>& view)
            : std::basic_istream<CharT>(nullptr), m_buf(view)
        {
            this->rdbuf(&m_buf);
        }
#else
        basic_istringviewstream(const std::basic_string<CharT>& str)
            : std::basic_istream<CharT>(nullptr), m_buf(str)
        {
            this->rdbuf(&m_buf);
        }
#endif // __cplusplus >= 201703L

    public:
        void str(const CharT* buffer, size_t size)
        {
            m_buf.str(buffer, size);
        }
#if __cplusplus >= 201703L
        void str(const std::basic_string_view<CharT>& view)
        {
            m_buf.str(view);
        }
#else
        void str(const std::basic_string<CharT>& str)
        {
            m_buf.str(str);
        }
#endif // __cplusplus >= 201703L

    private:
        stringviewbuffer<CharT> m_buf;
    };

    using istringviewstream = basic_istringviewstream<char>;
    using wistringviewstream = basic_istringviewstream<wchar_t>;
}

#endif // INPUT_STRING_VIEW_STREAM_H
