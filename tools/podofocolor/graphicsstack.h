/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _GRAPHICSSTACK_H_
#define _GRAPHICSSTACK_H_

#include <stack>
#include <podofo/podofo.h>

class GraphicsStack {

    class TGraphicsStackElement {
    public:
        TGraphicsStackElement()
            : m_strokingColor(PoDoFo::PdfColor(0.0)),
            m_nonStrokingColor(PoDoFo::PdfColor(0.0)),
            m_eColorSpaceStroking(PoDoFo::PdfColorSpaceType::DeviceGray),
            m_eColorSpaceNonStroking(PoDoFo::PdfColorSpaceType::DeviceGray)
        {
        }

        TGraphicsStackElement(const TGraphicsStackElement& rhs)
        {
            this->operator=(rhs);
        }

        const TGraphicsStackElement& operator=(const TGraphicsStackElement& rhs)
        {
            m_strokingColor = rhs.m_strokingColor;
            m_nonStrokingColor = rhs.m_nonStrokingColor;
            m_eColorSpaceStroking = rhs.m_eColorSpaceStroking;
            m_eColorSpaceNonStroking = rhs.m_eColorSpaceNonStroking;

            return *this;
        }

        inline const PoDoFo::PdfColor& GetStrokingColor() {
            return m_strokingColor;
        }

        inline const PoDoFo::PdfColor& GetNonStrokingColor() {
            return m_nonStrokingColor;
        }

        inline PoDoFo::PdfColorSpaceType GetStrokingColorSpace() {
            return m_eColorSpaceStroking;
        }

        inline PoDoFo::PdfColorSpaceType GetNonStrokingColorSpace() {
            return m_eColorSpaceNonStroking;
        }

        inline void SetStrokingColor(const PoDoFo::PdfColor& c) {
            m_strokingColor = c;
        }

        inline void SetNonStrokingColor(const PoDoFo::PdfColor& c) {
            m_nonStrokingColor = c;
        }

        inline void SetStrokingColorSpace(PoDoFo::PdfColorSpaceType eCS) {
            m_eColorSpaceStroking = eCS;
        }

        inline void SetNonStrokingColorSpace(PoDoFo::PdfColorSpaceType eCS) {
            m_eColorSpaceNonStroking = eCS;
        }

    private:
        PoDoFo::PdfColor m_strokingColor;
        PoDoFo::PdfColor m_nonStrokingColor;
        PoDoFo::PdfColorSpaceType m_eColorSpaceStroking;
        PoDoFo::PdfColorSpaceType m_eColorSpaceNonStroking;
    };

public:
    GraphicsStack();
    ~GraphicsStack();

    void Push();
    void Pop();

    inline const PoDoFo::PdfColor& GetStrokingColor() {
        return GetCurrentGraphicsState().GetStrokingColor();
    }

    inline const PoDoFo::PdfColor& GetNonStrokingColor() {
        return GetCurrentGraphicsState().GetNonStrokingColor();
    }

    inline PoDoFo::PdfColorSpaceType GetStrokingColorSpace() {
        return GetCurrentGraphicsState().GetStrokingColorSpace();
    }

    inline PoDoFo::PdfColorSpaceType GetNonStrokingColorSpace() {
        return GetCurrentGraphicsState().GetNonStrokingColorSpace();
    }

    inline void SetStrokingColor(const PoDoFo::PdfColor& c) {
        GetCurrentGraphicsState().SetStrokingColor(c);
    }

    inline void SetNonStrokingColor(const PoDoFo::PdfColor& c) {
        GetCurrentGraphicsState().SetNonStrokingColor(c);
    }

    inline void SetStrokingColorSpace(PoDoFo::PdfColorSpaceType eCS) {
        GetCurrentGraphicsState().SetStrokingColorSpace(eCS);
    }

    inline void SetNonStrokingColorSpace(PoDoFo::PdfColorSpaceType eCS) {
        GetCurrentGraphicsState().SetNonStrokingColorSpace(eCS);
    }

private:
    TGraphicsStackElement& GetCurrentGraphicsState();

private:
    std::stack<TGraphicsStackElement> m_stack;

};

#endif // _GRAPHICSSTACK_H_
