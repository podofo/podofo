/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfPage.h" 

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"
#include "base/PdfWriter.h"
#include "base/PdfStream.h"
#include "base/PdfColor.h"

#include "PdfDocument.h"

namespace PoDoFo {

PdfPage::PdfPage( const PdfRect & rSize, PdfDocument* pParent )
    : PdfElement( "Page", pParent ), PdfCanvas(), m_pContents( NULL )
{
    InitNewPage( rSize );
}

PdfPage::PdfPage( const PdfRect & rSize, PdfVecObjects* pParent )
    : PdfElement( "Page", pParent ), PdfCanvas(), m_pContents( NULL )
{
    InitNewPage( rSize );
}

PdfPage::PdfPage( PdfObject* pObject, const std::deque<PdfObject*> & rListOfParents )
    : PdfElement( "Page", pObject ), PdfCanvas()
{
    m_pResources = this->GetObject()->GetIndirectKey( "Resources" );
    if( !m_pResources ) 
    {
        // Resources might be inherited
        std::deque<PdfObject*>::const_reverse_iterator it = rListOfParents.rbegin();

        while( it != rListOfParents.rend() && !m_pResources )
        {
            m_pResources = (*it)->GetIndirectKey( "Resources" );
            ++it;
        }
    }

    PdfObject* pContents = this->GetObject()->GetIndirectKey( "Contents" );
    if (pContents)
    {
        m_pContents = new PdfContents( pContents );
    }
    else
    {
        // Create object on demand
        m_pContents =  NULL;
    }
}

PdfPage::~PdfPage()
{
    TIMapAnnotation ait, aend = m_mapAnnotations.end();

    for( ait = m_mapAnnotations.begin(); ait != aend; ait++ )
    {
        delete (*ait).second;
    }

    TIMapAnnotationDirect dit, dend = m_mapAnnotationsDirect.end();

    for( dit = m_mapAnnotationsDirect.begin(); dit != dend; dit++ )
    {
        delete (*dit).second;
    }

    delete m_pContents;	// just clears the C++ object from memory, NOT the PdfObject
}

void PdfPage::InitNewPage( const PdfRect & rSize )
{
    PdfVariant mediabox;
    rSize.ToVariant( mediabox );
    this->GetObject()->GetDictionary().AddKey( "MediaBox", mediabox );

    // The PDF specification suggests that we send all available PDF Procedure sets
    this->GetObject()->GetDictionary().AddKey( "Resources", PdfObject( PdfDictionary() ) );

    m_pResources = this->GetObject()->GetIndirectKey( "Resources" );
    m_pResources->GetDictionary().AddKey( "ProcSet", PdfCanvas::GetProcSet() );
}

void PdfPage::CreateContents() 
{
    if( !m_pContents ) 
    {
        m_pContents = new PdfContents( this );
        this->GetObject()->GetDictionary().AddKey( PdfName::KeyContents, 
                                                   m_pContents->GetContents()->Reference());   
    }
}

PdfObject* PdfPage::GetContents() const 
{ 
    if( !m_pContents ) 
    {
        const_cast<PdfPage*>(this)->CreateContents();
    }

    return m_pContents->GetContents(); 
}

PdfObject* PdfPage::GetContentsForAppending() const
{ 
    if( !m_pContents ) 
    {
        const_cast<PdfPage*>(this)->CreateContents();
    }

    return m_pContents->GetContentsForAppending(); 
}

PdfRect PdfPage::CreateStandardPageSize( const EPdfPageSize ePageSize, bool bLandscape )
{
    PdfRect rect;

    switch( ePageSize ) 
    {
        case ePdfPageSize_A0:
            rect.SetWidth( 2384.0 );
            rect.SetHeight( 3370.0 );
            break;

        case ePdfPageSize_A1:
            rect.SetWidth( 1684.0 );
            rect.SetHeight( 2384.0 );
            break;

        case ePdfPageSize_A2:
            rect.SetWidth( 1191.0 );
            rect.SetHeight( 1684.0 );
            break;
            
        case ePdfPageSize_A3:
            rect.SetWidth( 842.0 );
            rect.SetHeight( 1190.0 );
            break;

        case ePdfPageSize_A4:
            rect.SetWidth( 595.0 );
            rect.SetHeight( 842.0 );
            break;

        case ePdfPageSize_A5:
            rect.SetWidth( 420.0 );
            rect.SetHeight( 595.0 );
            break;

        case ePdfPageSize_A6:
            rect.SetWidth( 297.0 );
            rect.SetHeight( 420.0 );
            break;

        case ePdfPageSize_Letter:
            rect.SetWidth( 612.0 );
            rect.SetHeight( 792.0 );
            break;
            
        case ePdfPageSize_Legal:
            rect.SetWidth( 612.0 );
            rect.SetHeight( 1008.0 );
            break;

        case ePdfPageSize_Tabloid:
            rect.SetWidth( 792.0 );
            rect.SetHeight( 1224.0 );
            break;

        default:
            break;
    }

    if( bLandscape ) 
    {
        double dTmp = rect.GetWidth();
        rect.SetWidth ( rect.GetHeight() );
        rect.SetHeight(  dTmp );
    }

    return rect;
}

const PdfObject* PdfPage::GetInheritedKeyFromObject( const char* inKey, const PdfObject* inObject, int depth ) const
{
    const PdfObject* pObj = NULL;

    // check for it in the object itself
    if ( inObject->GetDictionary().HasKey( inKey ) ) 
    {
        pObj = inObject->MustGetIndirectKey( inKey );
        if ( !pObj->IsNull() ) 
            return pObj;
    }
    
    // if we get here, we need to go check the parent - if there is one!
    if( inObject->GetDictionary().HasKey( "Parent" ) ) 
    {
        // CVE-2017-5852 - prevent stack overflow if Parent chain contains a loop, or is very long
        // e.g. pObj->GetParent() == pObj or pObj->GetParent()->GetParent() == pObj
        // default stack sizes
        // Windows: 1 MB
        // Linux: 2 MB
        // macOS: 8 MB for main thread, 0.5 MB for secondary threads
        // 0.5 MB is enough space for 1000 512 byte stack frames and 2000 256 byte stack frames
        const int maxRecursionDepth = 1000;

        if ( depth > maxRecursionDepth )
            PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );

        pObj = inObject->GetIndirectKey( "Parent" );
        if( pObj == inObject )
        {
            std::ostringstream oss;
            oss << "Object " << inObject->Reference().ObjectNumber() << " "
                << inObject->Reference().GenerationNumber() << " references itself as Parent";
            PODOFO_RAISE_ERROR_INFO( ePdfError_BrokenFile, oss.str().c_str() );
        }

        if( pObj )
            pObj = GetInheritedKeyFromObject( inKey, pObj, depth + 1 );
    }

    return pObj;
}

const PdfRect PdfPage::GetPageBox( const char* inBox ) const
{
    PdfRect	 pageBox;
    const PdfObject*   pObj;
        
    // Take advantage of inherited values - walking up the tree if necessary
    pObj = GetInheritedKeyFromObject( inBox, this->GetObject() );
    
    // assign the value of the box from the array
    if ( pObj && pObj->IsArray() )
    {
        pageBox.FromArray( pObj->GetArray() );
    }
    else if ( strcmp( inBox, "ArtBox" ) == 0   ||
              strcmp( inBox, "BleedBox" ) == 0 ||
              strcmp( inBox, "TrimBox" ) == 0  )
    {
        // If those page boxes are not specified then
        // default to CropBox per PDF Spec (3.6.2)
        pageBox = GetPageBox( "CropBox" );
    }
    else if ( strcmp( inBox, "CropBox" ) == 0 )
    {
        // If crop box is not specified then
        // default to MediaBox per PDF Spec (3.6.2)
        pageBox = GetPageBox( "MediaBox" );
    }
    
    return pageBox;
}

int PdfPage::GetRotation() const 
{ 
    int rot = 0;
    
    const PdfObject* pObj = GetInheritedKeyFromObject( "Rotate", this->GetObject() ); 
    if ( pObj && pObj->IsNumber() )
        rot = static_cast<int>(pObj->GetNumber());
    
    return rot;
}

void PdfPage::SetRotation(int nRotation)
{
    if( nRotation != 0 && nRotation != 90 && nRotation != 180 && nRotation != 270 )
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );

    this->GetObject()->GetDictionary().AddKey( "Rotate", PdfVariant(static_cast<pdf_int64>(nRotation)) );
}

PdfObject* PdfPage::GetAnnotationsArray( bool bCreate ) const
{
    PdfObject* pObj;

    // check for it in the object itself
    if ( this->GetObject()->GetDictionary().HasKey( "Annots" ) ) 
    {
        pObj = this->GetObject()->GetIndirectKey( "Annots" );
        if( pObj && pObj->IsArray() )
            return pObj;
    }
    else if( bCreate ) 
    {
        PdfArray array;
        this->GetNonConstObject()->GetDictionary().AddKey( "Annots", array );
        return const_cast<PdfObject*>(this->GetObject()->GetDictionary().GetKey( "Annots" ));
    }

    return NULL;
}

int PdfPage::GetNumAnnots() const
{
    PdfObject* pObj = this->GetAnnotationsArray();

    return pObj ? static_cast<int>(pObj->GetArray().size()) : 0;
}

PdfAnnotation* PdfPage::CreateAnnotation( EPdfAnnotation eType, const PdfRect & rRect )
{
    PdfAnnotation* pAnnot = new PdfAnnotation( this, eType, rRect, this->GetObject()->GetOwner() );
    PdfObject*     pObj   = this->GetAnnotationsArray( true );
    PdfReference   ref    = pAnnot->GetObject()->Reference();

    pObj->GetArray().push_back( ref );
    m_mapAnnotations[ref] = pAnnot;

    return pAnnot;
}

PdfAnnotation* PdfPage::GetAnnotation( int index )
{
    PdfAnnotation* pAnnot;
    PdfReference   ref;

    PdfObject*     pObj   = this->GetAnnotationsArray( false );

    if( !(pObj && pObj->IsArray()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
    
    if( index < 0 && static_cast<unsigned int>(index) >= pObj->GetArray().size() )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    PdfObject* pItem = &(pObj->GetArray()[index]);
    if( pItem->IsDictionary() )
    {
        pAnnot = m_mapAnnotationsDirect[pItem];
        if( !pAnnot )
        {
            pAnnot = new PdfAnnotation( pItem, this );
            m_mapAnnotationsDirect[pItem] = pAnnot;
        }
    }
    else
    {
        ref = pItem->GetReference();
        pAnnot = m_mapAnnotations[ref];
        if( !pAnnot )
        {
            pObj = this->GetObject()->GetOwner()->GetObject( ref );
            if( !pObj )
            {
                PdfError::DebugMessage( "Error looking up object %i %i R\n", ref.ObjectNumber(), ref.GenerationNumber() );
                PODOFO_RAISE_ERROR( ePdfError_NoObject );
            }

            pAnnot = new PdfAnnotation( pObj, this );
            m_mapAnnotations[ref] = pAnnot;
        }
    }

    return pAnnot;
}

void PdfPage::DeleteAnnotation( int index )
{
    PdfObject* pObj = this->GetAnnotationsArray( false );
    PdfObject* pItem;

    if( !(pObj && pObj->IsArray()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if( index < 0 && static_cast<unsigned int>(index) >= pObj->GetArray().size() )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    pItem = &(pObj->GetArray()[index]);

    if( pItem->IsDictionary() )
    {
        PdfAnnotation* pAnnot;

        pObj->GetArray().erase( pObj->GetArray().begin() + index );

        // delete any cached PdfAnnotations
        pAnnot = m_mapAnnotationsDirect[pItem];
        if( pAnnot )
        {
            delete pAnnot;
            m_mapAnnotationsDirect.erase( pItem );
        }
    }
    else
    {
        this->DeleteAnnotation( pItem->GetReference() );
    }
}

void PdfPage::DeleteAnnotation( const PdfReference & ref )
{
    PdfAnnotation*     pAnnot;
    PdfArray::iterator it;
    PdfObject*         pObj   = this->GetAnnotationsArray( false );
    bool               bFound = false;

    // find the array iterator pointing to the annotation, so it can be deleted later

    if( !(pObj && pObj->IsArray()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    it = pObj->GetArray().begin();
    while( it != pObj->GetArray().end() ) 
    {
        if( (*it).IsReference() && (*it).GetReference() == ref ) 
        {
            // Element may not be deleted from the array at this point, because doing
            // this invalidates all PdfReferences references derived from the array.
            // This includes the 'ref' parameter, when it is never copied by value,
            // as it happens when this function is called via DeleteAnnotation( int )!
            bFound = true;
            break;
        }

        ++it;
    }

    // if no such annotation was found
    // throw an error instead of deleting
    // another object with this reference
    if( !bFound ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_NoObject );
    }

    // delete any cached PdfAnnotations
    pAnnot = m_mapAnnotations[ref];
    if( pAnnot )
    {
        delete pAnnot;
        m_mapAnnotations.erase( ref );
    }

    // delete the PdfObject in the file
    delete this->GetObject()->GetOwner()->RemoveObject( ref );
    
    // Delete the annotation from the annotation array.
	// Has to be performed at last, since it will invalidate 'ref' when
	// it was derived from the array itself and never copied by value!
    pObj->GetArray().erase( it );
}

// added by Petr P. Petrov 21 Febrary 2010
bool PdfPage::SetPageWidth(int newWidth)
{
    PdfObject*   pObjMediaBox;
        
    // Take advantage of inherited values - walking up the tree if necessary
    pObjMediaBox = const_cast<PdfObject*>(GetInheritedKeyFromObject( "MediaBox", this->GetObject() ));
    
    // assign the value of the box from the array
    if ( pObjMediaBox && pObjMediaBox->IsArray() )
    {
        // in PdfRect::FromArray(), the Left value is subtracted from Width
        double dLeftMediaBox = pObjMediaBox->GetArray()[0].GetReal();
        pObjMediaBox->GetArray()[2].SetReal( newWidth + dLeftMediaBox );

        PdfObject*   pObjCropBox;

        // Take advantage of inherited values - walking up the tree if necessary
        pObjCropBox = const_cast<PdfObject*>(GetInheritedKeyFromObject( "CropBox", this->GetObject() ));

        if ( pObjCropBox && pObjCropBox->IsArray() )
        {
            // in PdfRect::FromArray(), the Left value is subtracted from Width
            double dLeftCropBox = pObjCropBox->GetArray()[0].GetReal();
            pObjCropBox->GetArray()[2].SetReal( newWidth + dLeftCropBox );
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

// added by Petr P. Petrov 21 Febrary 2010
bool PdfPage::SetPageHeight(int newHeight)
{
    PdfObject*   pObj;
        
    // Take advantage of inherited values - walking up the tree if necessary
    pObj = const_cast<PdfObject*>(GetInheritedKeyFromObject( "MediaBox", this->GetObject() ));
    
    // assign the value of the box from the array
    if ( pObj && pObj->IsArray() )
    {
        // in PdfRect::FromArray(), the Bottom value is subtracted from Height
        double dBottom = pObj->GetArray()[1].GetReal();
        pObj->GetArray()[3].SetReal( newHeight + dBottom );

        PdfObject*   pObjCropBox;

        // Take advantage of inherited values - walking up the tree if necessary
        pObjCropBox = const_cast<PdfObject*>(GetInheritedKeyFromObject( "CropBox", this->GetObject() ));

        if ( pObjCropBox && pObjCropBox->IsArray() )
        {
        // in PdfRect::FromArray(), the Bottom value is subtracted from Height
            double dBottomCropBox = pObjCropBox->GetArray()[1].GetReal();
            pObjCropBox->GetArray()[3].SetReal( newHeight + dBottomCropBox );
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void PdfPage::SetTrimBox( const PdfRect & rSize )
{
    PdfVariant trimbox;
    rSize.ToVariant( trimbox );
    this->GetObject()->GetDictionary().AddKey( "TrimBox", trimbox );
}

unsigned int PdfPage::GetPageNumber() const
{
    unsigned int        nPageNumber = 0;
    PdfObject*          pParent     = this->GetObject()->GetIndirectKey( "Parent" );
    PdfReference ref                = this->GetObject()->Reference();

    // CVE-2017-5852 - prevent infinite loop if Parent chain contains a loop
    // e.g. pParent->GetIndirectKey( "Parent" ) == pParent or pParent->GetIndirectKey( "Parent" )->GetIndirectKey( "Parent" ) == pParent
    const int maxRecursionDepth = 1000;
    int depth = 0;

    while( pParent ) 
    {
        PdfObject* pKids = pParent->GetIndirectKey( "Kids" );
        if ( pKids != NULL )
        {
            const PdfArray& kids        = pKids->GetArray();
            PdfArray::const_iterator it = kids.begin();

            while( it != kids.end() && (*it).GetReference() != ref )
            {
                PdfObject* pNode = this->GetObject()->GetOwner()->GetObject( (*it).GetReference() );
                if (!pNode)
                {
                    std::ostringstream oss;
                    oss << "Object " << (*it).GetReference().ToString() << " not found from Kids array "
                        << pKids->Reference().ToString(); 
                    PODOFO_RAISE_ERROR_INFO( ePdfError_NoObject, oss.str() );
                }

                if( pNode->GetDictionary().HasKey( PdfName::KeyType )
                    && pNode->MustGetIndirectKey( PdfName::KeyType )->GetName() == PdfName( "Pages" ) )
                {
                    PdfObject* pCount = pNode->GetIndirectKey( "Count" );
                    if( pCount != NULL ) {
                        nPageNumber += static_cast<int>(pCount->GetNumber());
                    }
                } else {
                    // if we do not have a page tree node, 
                    // we most likely have a page object:
                    // so the page count is 1
                    ++nPageNumber;
                }
                ++it;
            }
        }

        ref     = pParent->Reference();
        pParent = pParent->GetIndirectKey( "Parent" );
        ++depth;

        if ( depth > maxRecursionDepth )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_BrokenFile, "Loop in Parent chain" );
        }
    }

    return ++nPageNumber;
}

int PdfPage::GetNumFields() const
{
    int                  nCount  = 0;
    int                  nAnnots = this->GetNumAnnots();
    const PdfAnnotation* pAnnot  = NULL;
    for( int i=0;i<nAnnots;i++ )
    {
        pAnnot = const_cast<PdfPage*>(this)->GetAnnotation( i );
        // Count every widget annotation with a FieldType as PdfField
        if( pAnnot->GetType() == ePdfAnnotation_Widget )
            ++nCount;
    }

    return nCount;
}

PdfField PdfPage::GetField( int index )
{
    int            nCount  = 0;
    int            nAnnots = this->GetNumAnnots();
    PdfAnnotation* pAnnot  = NULL;
    for( int i=0;i<nAnnots;i++ )
    {
        pAnnot = this->GetAnnotation( i );
        // Count every widget annotation with a FieldType as PdfField
        if( pAnnot->GetType() == ePdfAnnotation_Widget )
        {
            if( nCount == index )
            {
                return PdfField( pAnnot->GetObject(), pAnnot );
            }
            else
                ++nCount;
        }
    }

    PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
}

const PdfField PdfPage::GetField( int index ) const
{
    PdfField field = const_cast<PdfPage*>(this)->GetField( index );
    return field;
}

PdfObject* PdfPage::GetFromResources( const PdfName & rType, const PdfName & rKey )
{
    if( m_pResources == NULL ) // Fix CVE-2017-7381
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "No Resources" );
    } 
    if( m_pResources->GetDictionary().HasKey( rType ) ) 
    {
        // OC 15.08.2010 BugFix: Ghostscript creates here sometimes an indirect reference to a directory
     // PdfObject* pType = m_pResources->GetDictionary().GetKey( rType );
        PdfObject* pType = m_pResources->GetIndirectKey( rType );
        if( pType && pType->IsDictionary() && pType->GetDictionary().HasKey( rKey ) )
        {
            PdfObject* pObj = pType->GetDictionary().GetKey( rKey ); // CB 08.12.2017 Can be an array
            if (pObj->IsReference())
            {
                const PdfReference & ref = pType->GetDictionary().GetKey( rKey )->GetReference();
                return this->GetObject()->GetOwner()->GetObject( ref );
            }
            return pObj; // END
        }
    }
    
    return NULL;
}


PdfObject* PdfPage::GetOwnAnnotationsArray( bool bCreate, PdfDocument *pDocument)
{
   PdfObject* pObj;

   if ( this->GetObject()->GetDictionary().HasKey( "Annots" ) )  {
      pObj = this->GetObject()->GetIndirectKey( "Annots" );
        
      if(!pObj) {
         pObj = this->GetObject()->GetDictionary().GetKey("Annots");
         if( pObj->IsReference() ) {
            if( !pDocument ) {
               PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "Object is a reference but does not have an owner!" );
            }

            pObj = pDocument->GetObjects()->GetObject( pObj->GetReference() );
         }
      }

      if( pObj && pObj->IsArray() )
         return pObj;
    }
    else if( bCreate ) 
    {
        PdfArray array;
        this->GetNonConstObject()->GetDictionary().AddKey( "Annots", array );
        return const_cast<PdfObject*>(this->GetObject()->GetDictionary().GetKey( "Annots" ));
    }

    return NULL;
}

void PdfPage::SetICCProfile( const char *pszCSTag, PdfInputStream *pStream, pdf_int64 nColorComponents, EPdfColorSpace eAlternateColorSpace )
{
    // Check nColorComponents for a valid value
    if ( nColorComponents != 1 &&
         nColorComponents != 3 &&
         nColorComponents != 4 )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_ValueOutOfRange, "SetICCProfile nColorComponents must be 1, 3 or 4!" );
    }

    // Create a colorspace object
    PdfObject* iccObject = this->GetObject()->GetOwner()->CreateObject();
    PdfName nameForCS = PdfColor::GetNameForColorSpace( eAlternateColorSpace );
    iccObject->GetDictionary().AddKey( PdfName("Alternate"), nameForCS );
    iccObject->GetDictionary().AddKey( PdfName("N"), nColorComponents );
    iccObject->GetStream()->Set( pStream );

    // Add the colorspace
    PdfArray array;
    array.push_back( PdfName("ICCBased") );
    array.push_back( iccObject->Reference() );

    PoDoFo::PdfDictionary iccBasedDictionary;
    iccBasedDictionary.AddKey( PdfName(pszCSTag), array );

    // Add the colorspace to resource
    GetResources()->GetDictionary().AddKey( PdfName("ColorSpace"), iccBasedDictionary );
}


};
