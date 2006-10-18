
#ifndef _PDF_EXTGSTATE_H_
#define _PDF_EXTGSTATE_H_

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfElement.h"

namespace PoDoFo {

	class PdfObject;
	class PdfPage;
	class PdfWriter;

	/** This class wraps the ExtGState object used in the Resource
	 *  Dictionary of a Content-supporting element (page, Pattern, etc.)
	 *  The main usage is for transparency, but it also support a variety
	 *  of prepress features.
	*/
	class PdfExtGState : public PdfElement {
	public:
		/** Create a new PdfExtGState object which will introduce itself
		*  automatically to every page object it is used on.
		*
		*  \param pParent parent of the font object
		*  
		*/
		PdfExtGState( PdfVecObjects* pParent );
		virtual ~PdfExtGState();

		/** Returns the identifier of this ExtGState how it is known
		*  in the pages resource dictionary.
		*  \returns PdfName containing the identifier (e.g. /ExtGS13)
		*/
		inline const PdfName & GetIdentifier() const;

		/** Sets the opacity value to be used for fill operations
		 *  \param opac a floating point value from 0 (transparent) to 1 (opaque)
		*/
		void SetFillOpacity( float opac );

		/** Sets the opacity value to be used for stroking operations
		 *  \param opac a floating point value from 0 (transparent) to 1 (opaque)
		*/
		void SetStrokeOpacity( float opac );

	private:
		/** Initialize the object
		*/
		void Init( void );

	private: 
		PdfName m_Identifier;
	};


	const PdfName & PdfExtGState::GetIdentifier() const
	{
		return m_Identifier;
	}
};

#endif // _PDF_EXTGSTATE_H_

