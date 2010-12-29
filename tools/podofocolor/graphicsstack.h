/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _GRAPHICSSTACK_H_
#define _GRAPHICSSTACK_H_

#include <stack>
#include <podofo.h>

class GraphicsStack {

    class TGraphicsStackElement {
    public:
        TGraphicsStackElement()
            : m_strokingColor( PoDoFo::PdfColor( 0.0 ) ),
              m_nonStrokingColor( PoDoFo::PdfColor( 0.0 ) ),
              m_eColorSpaceStroking( PoDoFo::ePdfColorSpace_DeviceGray ),
              m_eColorSpaceNonStroking( PoDoFo::ePdfColorSpace_DeviceGray )
        {
        }

        TGraphicsStackElement( const TGraphicsStackElement & rhs ) 
        {
            this->operator=(rhs);
        }

        const TGraphicsStackElement & operator=( const TGraphicsStackElement & rhs ) 
        {
            m_strokingColor = rhs.m_strokingColor;
            m_nonStrokingColor = rhs.m_nonStrokingColor;
            m_eColorSpaceStroking = rhs.m_eColorSpaceStroking;
            m_eColorSpaceNonStroking = rhs.m_eColorSpaceNonStroking;

            return *this;
        }

        inline const PoDoFo::PdfColor & GetStrokingColor() {
            return m_strokingColor;
        }

        inline const PoDoFo::PdfColor & GetNonStrokingColor() {
            return m_nonStrokingColor;
        }

        inline PoDoFo::EPdfColorSpace GetStrokingColorSpace() {
            return m_eColorSpaceStroking;
        }

        inline PoDoFo::EPdfColorSpace GetNonStrokingColorSpace() {
            return m_eColorSpaceNonStroking;
        }

        inline void SetStrokingColor( const PoDoFo::PdfColor & c ) {
            m_strokingColor = c;
        }

        inline void SetNonStrokingColor( const PoDoFo::PdfColor & c ) {
            m_nonStrokingColor = c;
        }

        inline void SetStrokingColorSpace( PoDoFo::EPdfColorSpace eCS ) {
            m_eColorSpaceStroking = eCS;
        }

        inline void SetNonStrokingColorSpace( PoDoFo::EPdfColorSpace eCS ) {
            m_eColorSpaceNonStroking = eCS;
        }

    private:
        PoDoFo::PdfColor m_strokingColor;
        PoDoFo::PdfColor m_nonStrokingColor;
        PoDoFo::EPdfColorSpace m_eColorSpaceStroking;
        PoDoFo::EPdfColorSpace m_eColorSpaceNonStroking;
    };

public:
    GraphicsStack();
    ~GraphicsStack();

    void Push();
    void Pop();

    inline const PoDoFo::PdfColor & GetStrokingColor() {
        return GetCurrentGraphicsState().GetStrokingColor();
    }

    inline const PoDoFo::PdfColor & GetNonStrokingColor() {
        return GetCurrentGraphicsState().GetNonStrokingColor();
    }
    
    inline PoDoFo::EPdfColorSpace GetStrokingColorSpace() {
        return GetCurrentGraphicsState().GetStrokingColorSpace();
    }

    inline PoDoFo::EPdfColorSpace GetNonStrokingColorSpace() {
        return GetCurrentGraphicsState().GetNonStrokingColorSpace();
    }
    
    inline void SetStrokingColor( const PoDoFo::PdfColor & c ) {
        GetCurrentGraphicsState().SetStrokingColor( c );
    }
    
    inline void SetNonStrokingColor( const PoDoFo::PdfColor & c ) {
        GetCurrentGraphicsState().SetNonStrokingColor( c );
    }
    
    inline void SetStrokingColorSpace( PoDoFo::EPdfColorSpace eCS ) {
        GetCurrentGraphicsState().SetStrokingColorSpace( eCS );
    }

    inline void SetNonStrokingColorSpace( PoDoFo::EPdfColorSpace eCS ) {
        GetCurrentGraphicsState().SetNonStrokingColorSpace( eCS );
    }
    
private:
    TGraphicsStackElement & GetCurrentGraphicsState(); 

private:
    std::stack<TGraphicsStackElement> m_stack;

};

#endif // _GRAPHICSSTACK_H_
