#ifndef OUT_STRING_STREAM_H
#define OUT_STRING_STREAM_H
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

#include <cstdlib>
#include <limits>
#include <ostream>
#include <string>
#if __cplusplus >= 201703L
#include <string_view>
#endif

namespace cmn
{
    // Inspired from Kuba's post https://stackoverflow.com/a/38862462/213871
    template <typename CharT>
    class basic_outstringstream : public std::basic_ostream<CharT, std::char_traits<CharT>>
    {
        using traits_type = std::char_traits<CharT>;
        using base_stream_type = std::basic_ostream<CharT, traits_type>;

        class buffer : public std::basic_streambuf<CharT, std::char_traits<CharT>>
        {
            using base_buf_type = std::basic_streambuf<CharT, traits_type>;
            using int_type = typename base_buf_type::int_type;

        private:
            void safe_pbump(std::streamsize off)
            {
                // pbump doesn't support 64 bit offsets
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47921
                int maxbump;
                if (off > 0)
                    maxbump = std::numeric_limits<int>::max();
                else if (off < 0)
                    maxbump = std::numeric_limits<int>::min();
                else // == 0
                    return;

                while (std::abs(off) > std::numeric_limits<int>::max())
                {
                    this->pbump(maxbump);
                    off -= maxbump;
                }

                this->pbump((int)off);
            }

            void init()
            {
                this->setp(const_cast<CharT *>(m_str.data()),
                    const_cast<CharT *>(m_str.data()) + m_str.size());
                this->safe_pbump((std::streamsize)m_str.size());
            }

        protected:
            int_type overflow(int_type ch) override
            {
                if (traits_type::eq_int_type(ch, traits_type::eof()))
                    return traits_type::not_eof(ch);

                if (m_str.empty())
                    m_str.resize(1);
                else
                    m_str.resize(m_str.size() * 2);

                size_t size = this->size();
                this->setp(const_cast<CharT *>(m_str.data()),
                    const_cast<CharT *>(m_str.data()) + m_str.size());
                this->safe_pbump((std::streamsize)size);
                *this->pptr() = traits_type::to_char_type(ch);
                this->pbump(1);

                return ch;
            }

        public:
            buffer(std::size_t reserveSize)
            {
                m_str.reserve(reserveSize);
                init();
            }

            buffer(std::basic_string<CharT>&& str)
                : m_str(std::move(str))
            {
                init();
            }

            buffer(const std::basic_string<CharT>& str)
                : m_str(str)
            {
                init();
            }

        public:
            size_t size() const
            {
                return (size_t)(this->pptr() - this->pbase());
            }

#if __cplusplus >= 201703L
            std::basic_string_view<CharT> str() const
            {
                return std::basic_string_view<CharT>(m_str.data(), size());
            }
#endif
            std::basic_string<CharT> take_str()
            {
                // Resize the string to actual used buffer size
                m_str.resize(size());
                std::string ret = std::move(m_str);
                init();
                return ret;
            }

            void clear()
            {
                m_str.clear();
                init();
            }

            const CharT * data() const
            {
                return m_str.data();
            }

        private:
            std::basic_string<CharT> m_str;
        };

    public:
        explicit basic_outstringstream(std::size_t reserveSize = 8)
            : base_stream_type(nullptr), m_buffer(reserveSize)
        {
            this->rdbuf(&m_buffer);
        }

        explicit basic_outstringstream(std::basic_string<CharT>&& str)
            : base_stream_type(nullptr), m_buffer(str)
        {
            this->rdbuf(&m_buffer);
        }

        explicit basic_outstringstream(const std::basic_string<CharT>& str)
            : base_stream_type(nullptr), m_buffer(str)
        {
            this->rdbuf(&m_buffer);
        }

#if __cplusplus >= 201703L
        std::basic_string_view<CharT> str() const
        {
            return m_buffer.str();
        }
#endif
        std::basic_string<CharT> take_str()
        {
            return m_buffer.take_str();
        }

        const CharT * data() const
        {
            return m_buffer.data();
        }

        size_t size() const
        {
            return m_buffer.size();
        }

        void clear()
        {
            m_buffer.clear();
        }

    private:
        buffer m_buffer;
    };

    using outstringstream = basic_outstringstream<char>;
    using woutstringstream = basic_outstringstream<wchar_t>;
}

#endif // OUT_STRING_STREAM_H
