#ifndef UTFCPP_EXTENSIONS_H
#define UTFCPP_EXTENSIONS_H

#include <cstddef>
#include <stdexcept>
#include <utf8cpp/utf8.h>

namespace utf8
{
    /** Create an iterable structure that will yield characters
     * from an octet buffer encoded as an unaligned big-endian
     * utf-16 string
     */
    template <typename ByteT>
    class u16beoctetiterable final
    {
    public:
        u16beoctetiterable(const ByteT* buffer, size_t size, bool unchecked = false)
            : m_buffer(buffer), m_size(size)
        {
            if (size % sizeof(uint16_t) == 1)
            {
                if (unchecked)
                    m_size -= 1;
                else
                    throw std::range_error("Invalid utf16 range");
            }
        }

    public:
        class iterator final
        {
            friend class u16beoctetiterable;
        public:
            using difference_type = void;
            using value_type = uint16_t;
            using pointer = void;
            using reference = void;
            using iterator_category = std::forward_iterator_tag;
        private:
            iterator(const ByteT* curr)
                : m_curr(curr) { }
        public:
            iterator(const iterator&) = default;
            iterator& operator=(const iterator&) = default;
            bool operator==(const iterator& rhs) const
            {
                return m_curr == rhs.m_curr;
            }
            bool operator!=(const iterator& rhs) const
            {
                return m_curr != rhs.m_curr;
            }
            iterator& operator++()
            {
                m_curr += 2;
                return *this;
            }
            iterator operator++(int)
            {
                auto copy = *this;
                m_curr += 2;
                return copy;
            }
            uint16_t operator*()
            {
                return (uint16_t)((uint8_t)*m_curr << 8 | (uint8_t) * (m_curr + 1));
            }
        private:
            const ByteT *m_curr;
        };

    public:
        iterator begin() const
        {
            return iterator(m_buffer);
        }
        iterator end() const
        {
            return iterator(m_buffer + m_size);
        }

    private:
        const ByteT* m_buffer;
        size_t m_size;
    };

    /** Create an iterable structure that will yield characters
     * from an octet buffer encoded as an unaligned little-endian
     * utf-16 string
     */
    template <typename ByteT>
    class u16leoctetiterable final
    {
    public:
        u16leoctetiterable(const ByteT* buffer, size_t size, bool unchecked = false)
            : m_buffer(buffer), m_size(size)
        {
            if (size % sizeof(uint16_t) == 1)
            {
                if (unchecked)
                    m_size -= 1;
                else
                    throw std::range_error("Invalid utf16 range");
            }
        }

    public:
        class iterator final
        {
            friend class u16leoctetiterable;
        public:
            using difference_type = void;
            using value_type = uint16_t;
            using pointer = void;
            using reference = void;
            using iterator_category = std::forward_iterator_tag;
        private:
            iterator(const ByteT* curr)
                : m_curr(curr) { }
        public:
            iterator(const iterator&) = default;
            iterator& operator=(const iterator&) = default;
            bool operator==(const iterator& rhs) const
            {
                return m_curr == rhs.m_curr;
            }
            bool operator!=(const iterator& rhs) const
            {
                return m_curr != rhs.m_curr;
            }
            iterator& operator++()
            {
                m_curr += 2;
                return *this;
            }
            iterator operator++(int)
            {
                auto copy = *this;
                m_curr += 2;
                return copy;
            }
            uint16_t operator*()
            {
                return (uint16_t)((uint8_t)*(m_curr + 1) << 8 | (uint8_t)*m_curr);
            }
        private:
            const ByteT* m_curr;
        };

    public:
        iterator begin() const
        {
            return iterator(m_buffer);
        }
        iterator end() const
        {
            return iterator(m_buffer + m_size);
        }

    private:
        const ByteT* m_buffer;
        size_t m_size;
    };

    using u16bechariterable = u16beoctetiterable<char>;
    using u16lechariterable = u16leoctetiterable<char>;

    template <typename word_iterator>
    word_iterator append16(uint32_t cp, word_iterator result)
    {
        if (!utf8::internal::is_code_point_valid(cp))
            throw invalid_code_point(cp);

        if (cp < 0x10000u) {                    // one word
            *(result++) = static_cast<uint16_t>(cp);
        }
        else {                                  // two words
            uint32_t cp_1 = cp - 0x10000u;
            *(result++) = static_cast<uint16_t>(cp_1 / 0x400u + 0xd800u);
            *(result++) = static_cast<uint16_t>(cp_1 % 0x400u + 0xdc00u);
        }

        return result;
    }

    namespace internal
    {
        template <typename u16bit_iterator>
        utf_error utf16_validate_next(u16bit_iterator& start, u16bit_iterator end, uint32_t& code_point)
        {
            uint32_t cp = utf8::internal::mask16(*start++);
            if (utf8::internal::is_lead_surrogate(cp))
            {
                if (start != end)
                {
                    uint32_t trail_surrogate = utf8::internal::mask16(*start++);
                    if (utf8::internal::is_trail_surrogate(trail_surrogate))
                        cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
                    else
                        return utf_error::INVALID_CODE_POINT;
                }
                else
                    return utf_error::INCOMPLETE_SEQUENCE;
            }
            // Lone trail surrogate
            else if (utf8::internal::is_trail_surrogate(cp))
                return utf_error::INVALID_LEAD;

            code_point = cp;
            return utf_error::UTF8_OK;
        }
    }

    template <typename u16bit_iterator, typename octet_iterator>
    octet_iterator utf16to8_lenient(u16bit_iterator start, u16bit_iterator end, octet_iterator result)
    {
        uint32_t cp;
        while (start != end)
        {
            auto err_code = internal::utf16_validate_next(start, end, cp);
            if (err_code != internal::utf_error::UTF8_OK)
                return result;

            result = unchecked::append(cp, result);
        }

        return result;
    }

    namespace unchecked
    {
        template <typename word_iterator>
        word_iterator append16(uint32_t cp, word_iterator result)
        {
            if (cp < 0x10000u) {                    // one word
                *(result++) = static_cast<uint16_t>(cp);
            }
            else {                                  // two words
                uint32_t cp_1 = cp - 0x10000u;
                *(result++) = static_cast<uint16_t>(cp_1 / 0x400u + 0xd800u);
                *(result++) = static_cast<uint16_t>(cp_1 % 0x400u + 0xdc00u);
            }

            return result;
        }
    }

    inline void append(char32_t cp, std::u16string& s)
    {
        append16(uint32_t(cp), std::back_inserter(s));
    }
}

#endif // UTFCPP_EXTENSIONS_H
