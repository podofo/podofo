#ifndef UTFCPP_EXTENSIONS_H
#define UTFCPP_EXTENSIONS_H

#include <cstddef>
#include <stdexcept>

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
        u16beoctetiterable(const ByteT* buffer, size_t size, bool checked = true)
            : m_buffer(buffer), m_size(size)
        {
            if (size % sizeof(uint16_t) == 1)
            {
                if (checked)
                    throw std::range_error("Invalid utf16 range");
                else
                    m_size -= 1;
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
        u16leoctetiterable(const ByteT* buffer, size_t size, bool checked = true)
            : m_buffer(buffer), m_size(size)
        {
            if (size % sizeof(uint16_t) == 1)
            {
                if (checked)
                    throw std::range_error("Invalid utf16 range");
                else
                    m_size -= 1;
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
