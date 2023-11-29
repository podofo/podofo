/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AUX_OUTPUT_STREAM_H
#define AUX_OUTPUT_STREAM_H

#include "basedefs.h"

namespace PoDoFo {

/** An interface for writing blocks of data to
 *  a data source.
 */
class PODOFO_API OutputStream
{
public:
    OutputStream();
    virtual ~OutputStream();

    /** Write the character in the device
     *
     *  \param ch the character to write
     */
    void Write(char ch);

    /** Write the view to the OutputStream
     *
     *  \param view the view to be written
     */
    void Write(const std::string_view& view);

    /** Write data to the output stream
     *
     *  \param buffer the data is read from this buffer
     *  \param len    the size of the buffer
     */
    void Write(const char* buffer, size_t size);

    void Flush();

protected:
    static void WriteBuffer(OutputStream& stream, const char* buffer, size_t size);
    static void Flush(OutputStream& stream);

protected:
    virtual void writeBuffer(const char* buffer, size_t size) = 0;
    virtual void flush();

    /** Optional checks before writing
     * By default does nothing
     */
    virtual void checkWrite() const;

private:
    OutputStream(const OutputStream&) = delete;
    OutputStream& operator=(const OutputStream&) = delete;
};

};

#endif // AUX_OUTPUT_STREAM_H
