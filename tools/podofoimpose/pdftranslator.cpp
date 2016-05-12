/***************************************************************************
 *   Copyright (C) 2007 by Pierre Marchand   *
 *   pierre@moulindetouvois.com   *
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
#include "pdftranslator.h"
// #include "charpainter.h"
#include "planreader_legacy.h"

#ifdef PODOFO_HAVE_LUA
#include "planreader_lua.h"
#endif

#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
#include <cstdlib>
using std::ostringstream;
using std::map;
using std::vector;
using std::string;
using std::ifstream;
using std::istream;
using std::ostream;
using std::endl;
using std::runtime_error;

#include <iostream> //XXX
namespace PoDoFo
{
	namespace Impose
	{

#define MAX_SOURCE_PAGES 5000
#define MAX_RECORD_SIZE 2048



		bool PdfTranslator::checkIsPDF ( std::string path )
		{
			ifstream in ( path.c_str(), ifstream::in );
			if ( !in.good() )
				throw runtime_error ( "setSource() failed to open input file" );

			const int magicBufferLen = 5;
			char magicBuffer[magicBufferLen ];
			in.read ( magicBuffer, magicBufferLen );
			std::string magic ( magicBuffer , magicBufferLen );

			in.close();
			if ( magic.find ( "%PDF" ) < 5 )
				return true;
// 			throw runtime_error("First bytes of the file tend to indicate it is not a PDF file");
			return false;
		}

		PdfTranslator::PdfTranslator ( )
		{
			std::cerr<<"PdfTranslator::PdfTranslator"<<std::endl;
			sourceDoc = 0;
			targetDoc = 0;
			extraSpace = 0;
			scaleFactor = 1.0;
		}

		void PdfTranslator::setSource ( const std::string & source )
		{
			int dbg(0);
// 			std::cerr<<"PdfTranslator::setSource "<<source<<std::endl;
			std::cerr<< ++dbg <<std::endl;
			if ( checkIsPDF ( source ) )
			{
// 		std::cerr << "Appending "<<source<<" to source" << endl;
				multiSource.push_back ( source );
			}
			else
			{

				ifstream in ( source.c_str(), ifstream::in );
				if ( !in.good() )
					throw runtime_error ( "setSource() failed to open input file" );


				char *filenameBuffer = new char[1000];
				do
				{
					in.getline ( filenameBuffer, 1000 );
					std::string ts ( filenameBuffer, in.gcount() );
					if ( ts.size() > 4 ) // at least ".pdf" because just test if ts is empty doesn't work.
					{
						multiSource.push_back ( ts );
						std::cerr << "Appending "<< ts <<" to source" << endl;
					}
				}
				while ( !in.eof() );
				in.close();
				delete filenameBuffer;
			}
			std::cerr<< ++dbg <<std::endl;

			for ( std::vector<std::string>::const_iterator ms = multiSource.begin(); ms != multiSource.end(); ++ms )
			{
				if ( ms == multiSource.begin() )
				{
// 					std::cerr << "First doc is "<< (*ms).c_str()   << endl;
					try{
						sourceDoc = new PdfMemDocument ( ( *ms ).c_str() );
					}
					catch(PdfError& e)
					{
						std::cerr<<"Unable to create Document: " <<PdfError::ErrorMessage( e. GetError() )<<endl;
						return;
					}
				}
				else
				{
					PdfMemDocument mdoc ( ( *ms ).c_str() );
// 			std::cerr << "Appending "<< mdoc.GetPageCount() << " page(s) of " << *ms  << endl;
					sourceDoc->InsertPages ( mdoc, 0, mdoc.GetPageCount() );
				}
			}

			pcount = sourceDoc->GetPageCount();
// 	std::cerr << "Document has "<< pcount << " page(s) " << endl;
			if ( pcount > 0 ) // only here to avoid possible segfault, but PDF without page is not conform IIRC
			{
				PoDoFo::PdfRect rect ( sourceDoc->GetPage ( 0 )->GetMediaBox() );
				// keep in mind it’s just a hint since PDF can have different page sizes in a same doc
				sourceWidth =  rect.GetWidth() - rect.GetLeft();
				sourceHeight =  rect.GetHeight() - rect.GetBottom() ;
			}
		}

		void PdfTranslator::addToSource ( const std::string & source )
		{
// 			std::cerr<<"PdfTranslator::addToSource "<< source<<std::endl;
			if ( !sourceDoc )
				return;

			PdfMemDocument extraDoc ( source.c_str() );
			sourceDoc->InsertPages ( extraDoc, 0,  extraDoc.GetPageCount() );
			multiSource.push_back ( source );

		}

		PdfObject* PdfTranslator::migrateResource ( PdfObject * obj )
		{
// 			std::cerr<<"PdfTranslator::migrateResource"<<std::endl;
			PdfObject *ret ( 0 );

			if ( obj->IsDictionary() )
			{
				ret = targetDoc->GetObjects().CreateObject ( *obj );

				TKeyMap resmap = obj->GetDictionary().GetKeys();
				for ( TCIKeyMap itres = resmap.begin(); itres != resmap.end(); ++itres )
				{
					PdfObject *o = itres->second;
					ret->GetDictionary().AddKey ( itres->first , migrateResource ( o ) );
				}

				if ( obj->HasStream() )
				{
					* ( ret->GetStream() ) = * ( obj->GetStream() );
				}
			}
			else if ( obj->IsArray() )
			{
				PdfArray carray ( obj->GetArray() );
				PdfArray narray;
				for ( unsigned int ci = 0; ci < carray.GetSize(); ++ci )
				{
					PdfObject *co ( migrateResource ( &carray[ci] ) );
					narray.push_back ( *co );
				}
				ret = targetDoc->GetObjects().CreateObject ( narray );
			}
			else if ( obj->IsReference() )
			{
				if ( migrateMap.find ( obj->GetReference().ToString() ) != migrateMap.end() )
				{
					return migrateMap[obj->GetReference().ToString() ];
				}

				PdfObject * o ( migrateResource ( sourceDoc->GetObjects().GetObject ( obj->GetReference() ) ) );

                                ret  = new PdfObject ( o->Reference() ) ;

			}
			else
			{
				ret = new PdfObject ( *obj );//targetDoc->GetObjects().CreateObject(*obj);
			}


			migrateMap.insert ( std::pair<std::string, PdfObject*> ( obj->Reference().ToString(), ret ) );


			return ret;

		}

		PdfObject* PdfTranslator::getInheritedResources ( PdfPage* page )
		{
// 			std::cerr<<"PdfTranslator::getInheritedResources"<<std::endl;
			PdfObject *res ( 0 ); // = new PdfObject;
			PdfObject *rparent = page->GetObject();
			while ( rparent && rparent->IsDictionary() )
			{
				PdfObject *curRes = rparent->GetDictionary().GetKey ( PdfName ( "Resources" ) );
				if ( curRes )
				{
					res = migrateResource ( curRes );
				}
				rparent = rparent->GetIndirectKey ( "Parent" );
			}
			return res;

		}

		void PdfTranslator::setTarget ( const std::string & target )
		{
// 			std::cerr<<"PdfTranslator::setTarget "<<target<<std::endl;
			if ( !sourceDoc )
				throw std::logic_error ( "setTarget() called before setSource()" );

			targetDoc = new PdfMemDocument;
			outFilePath  = target;

			for ( int i = 0; i < pcount ; ++i )
			{
				PdfPage * page = sourceDoc->GetPage ( i );
				PdfMemoryOutputStream outMemStream ( 1 );

				PdfXObject *xobj = new PdfXObject ( page->GetMediaBox(), targetDoc );
				if ( page->GetContents()->HasStream() )
				{
					page->GetContents()->GetStream()->GetFilteredCopy ( &outMemStream );
				}
				else if ( page->GetContents()->IsArray() )
				{
					PdfArray carray ( page->GetContents()->GetArray() );
					for ( unsigned int ci = 0; ci < carray.GetSize(); ++ci )
					{
						if ( carray[ci].HasStream() )
						{
							carray[ci].GetStream()->GetFilteredCopy ( &outMemStream );
						}
						else if ( carray[ci].IsReference() )
						{
							PdfObject *co = sourceDoc->GetObjects().GetObject ( carray[ci].GetReference() );

							while ( co != NULL )
							{
								if ( co->IsReference() )
								{
									co = sourceDoc->GetObjects().GetObject ( co->GetReference() );
								}
								else if ( co->HasStream() )
								{
									co->GetStream()->GetFilteredCopy ( &outMemStream );
									break;
								}
							}

						}

					}
				}

				/// Its time to manage other keys of the page dictionary.
				std::vector<std::string> pageKeys;
				std::vector<std::string>::const_iterator itKey;
				pageKeys.push_back ( "Group" );
				for ( itKey = pageKeys.begin(); itKey != pageKeys.end(); ++itKey )
				{
					PoDoFo::PdfName keyname ( *itKey );
					if ( page->GetObject()->GetDictionary().HasKey ( keyname ) )
					{
						xobj->GetObject()->GetDictionary().AddKey ( keyname, migrateResource ( page->GetObject()->GetDictionary().GetKey ( keyname ) ) );
					}
				}

				outMemStream.Close();

				PdfMemoryInputStream inStream ( outMemStream.TakeBuffer(),outMemStream.GetLength() );
				xobj->GetContents()->GetStream()->Set ( &inStream );

				resources[i+1] = getInheritedResources ( page );
				xobjects[i+1] = xobj;
				cropRect[i+1] = page->GetCropBox();
				bleedRect[i+1] = page->GetBleedBox();
				trimRect[i+1] = page->GetTrimBox();
				artRect[i+1] = page->GetArtBox();

			}


			targetDoc->SetPdfVersion ( sourceDoc->GetPdfVersion() );

			PdfInfo *sInfo ( sourceDoc->GetInfo() );
			PdfInfo *tInfo ( targetDoc->GetInfo() );

			if ( sInfo->GetAuthor() != PdfString::StringNull )
				tInfo->SetAuthor ( sInfo->GetAuthor() );
			if ( sInfo->GetCreator() != PdfString::StringNull )
				tInfo->SetCreator ( sInfo->GetCreator() );
			if ( sInfo->GetSubject() != PdfString::StringNull )
				tInfo->SetSubject ( sInfo->GetSubject() );
			if ( sInfo->GetTitle() != PdfString::StringNull )
				tInfo->SetTitle ( sInfo->GetTitle() );
			if ( sInfo->GetKeywords() != PdfString::StringNull )
				tInfo->SetKeywords ( sInfo->GetKeywords() );

			if ( sInfo->GetTrapped() != PdfName::KeyNull )
				tInfo->SetTrapped ( sInfo->GetTrapped() );


// 	PdfObject *scat( sourceDoc->GetCatalog() );
// 	PdfObject *tcat( targetDoc->GetCatalog() );
// 	TKeyMap catmap = scat->GetDictionary().GetKeys();
// 	for ( TCIKeyMap itc = catmap.begin(); itc != catmap.end(); ++itc )
// 	{
// 		if(tcat->GetDictionary().GetKey(itc->first) == 0)
// 		{
// 			PdfObject *o = itc->second;
// 			tcat->GetDictionary().AddKey (itc->first , migrateResource( o ) );
// 		}
// 	}

// 	delete sourceDoc;
		}

		void PdfTranslator::loadPlan ( const std::string & planFile , PoDoFo::Impose::PlanReader loader )
		{
// 			std::cerr<< "loadPlan" << planFile<<std::endl;
			SourceVars sv;
			sv.PageCount = pcount;
			sv.PageHeight = sourceHeight;
			sv.PageWidth = sourceWidth;
			planImposition = new ImpositionPlan ( sv );
			if ( loader == PoDoFo::Impose::Legacy )
			{
				PlanReader_Legacy ( planFile, planImposition );
			}
#if defined(PODOFO_HAVE_LUA)
			else if ( loader == PoDoFo::Impose::Lua )
			{
				PlanReader_Lua ( planFile, planImposition );
			}
#endif

			if ( !planImposition->valid() )
				throw std::runtime_error ( "Unable to build a valid imposition plan" );

			destWidth = planImposition->destWidth();
			destHeight = planImposition->destHeight();
			scaleFactor = planImposition->scale();
			boundingBox = planImposition->boundingBox();
// 	std::cerr <<"Plan completed "<< planImposition.size() <<endl;

		}

		void PdfTranslator::impose()
		{
// 			std::cerr<<"PdfTranslator::impose"<<std::endl;
			if ( !targetDoc )
				throw std::invalid_argument ( "impose() called with empty target" );

//			PdfObject trimbox;
//			PdfRect trim ( 0, 0, destWidth, destHeight );
//			trim.ToVariant ( trimbox );
			std::map<int, PdfRect>* bbIndex = NULL;
			if(boundingBox.size() > 0)
			{
				if(boundingBox.find("crop") != std::string::npos)
				{
					bbIndex = &cropRect;
				}
				else if(boundingBox.find("bleed") != std::string::npos)
				{
					bbIndex = &bleedRect;
				}
				else if(boundingBox.find("trim") != std::string::npos)
				{
					bbIndex = &trimRect;
				}
				else if(boundingBox.find("art") != std::string::npos)
				{
					bbIndex = &artRect;
				}
			}

			typedef map<int, vector<PageRecord> > groups_t;
			groups_t groups;
			for ( unsigned int i = 0; i < planImposition->size(); ++i )
			{
				groups[ ( *planImposition ) [i].destPage].push_back ( ( *planImposition ) [i] );
			}
			
			unsigned int lastPlate(0);
			groups_t::const_iterator  git = groups.begin();
			const groups_t::const_iterator gitEnd = groups.end();
			while ( git != gitEnd )
			{
				PdfPage * newpage = NULL;
				// Allow "holes" in dest. pages sequence.
				unsigned int curPlate(git->first);
				while(lastPlate != curPlate)
				{
					newpage = targetDoc->CreatePage ( PdfRect ( 0.0, 0.0, destWidth, destHeight ) );
					++lastPlate;
				}
// 		newpage->GetObject()->GetDictionary().AddKey ( PdfName ( "TrimBox" ), trimbox );
				PdfDictionary xdict;

				ostringstream buffer;
				// Scale
				buffer << std::fixed << scaleFactor <<" 0 0 "<< scaleFactor <<" 0 0 cm\n";

				for ( unsigned int i = 0; i < git->second.size(); ++i )
				{
					PageRecord curRecord ( git->second[i] );
// 					std::cerr<<curRecord.sourcePage<< " " << curRecord.destPage<<std::endl;
					if(curRecord.sourcePage <= pcount)
					{
						double cosR = cos ( curRecord.rotate  *  3.14159 / 180.0 );
						double sinR = sin ( curRecord.rotate  *  3.14159 / 180.0 );
						double tx = curRecord.transX ;
						double ty = curRecord.transY ;
	
						int resourceIndex ( /*(curRecord.duplicateOf > 0) ? curRecord.duplicateOf : */curRecord.sourcePage );
						PdfXObject *xo = xobjects[resourceIndex];
						if(NULL != bbIndex)
						{
							PdfObject bb;
							// DominikS: Fix compilation using Visual Studio on Windows
							// bbIndex->at(resourceIndex).ToVariant( bb );							
							((*bbIndex)[resourceIndex]).ToVariant( bb );
							xo->GetObject()->GetDictionary().AddKey ( PdfName ( "BBox" ), bb );
						}
						ostringstream op;
						op << "OriginalPage" << resourceIndex;
						xdict.AddKey ( PdfName ( op.str() ) , xo->GetObjectReference() );
	
						if ( resources[resourceIndex] )
						{
							if ( resources[resourceIndex]->IsDictionary() )
							{
								TKeyMap resmap = resources[resourceIndex]->GetDictionary().GetKeys();
								TCIKeyMap itres;
								for ( itres = resmap.begin(); itres != resmap.end(); ++itres )
								{
									xo->GetResources()->GetDictionary().AddKey ( itres->first, itres->second );
								}
							}
							else if ( resources[resourceIndex]->IsReference() )
							{
								xo->GetObject()->GetDictionary().AddKey ( PdfName ( "Resources" ), resources[resourceIndex] );
							}
							else
								std::cerr<<"ERROR Unknown type resource "<<resources[resourceIndex]->GetDataTypeString()  <<  std::endl;
	
						}
						// Very primitive but it makes it easy to track down imposition plan into content stream.
						buffer << "q\n";
						buffer << std::fixed << cosR <<" "<< sinR<<" "<<-sinR<<" "<< cosR<<" "<< tx <<" "<<  ty << " cm\n";
						buffer << "/OriginalPage" << resourceIndex << " Do\n";
						buffer << "Q\n";
					}
				}
				if (!newpage)
					PODOFO_RAISE_ERROR (ePdfError_ValueOutOfRange);
				string bufStr = buffer.str();
				newpage->GetContentsForAppending()->GetStream()->Set ( bufStr.data(), bufStr.size() );
				newpage->GetResources()->GetDictionary().AddKey ( PdfName ( "XObject" ), xdict );
				++git;
			}

			targetDoc->Write ( outFilePath.c_str() );

		}


	};
}; // end of namespace
