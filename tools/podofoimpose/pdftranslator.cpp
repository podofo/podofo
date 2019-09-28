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
			sourceDoc = NULL;
			targetDoc = NULL;
			planImposition = NULL;
			duplicate = 0;
			extraSpace = 0;
			scaleFactor = 1.0;
			pcount = 0;
			sourceWidth = 0.0;
			sourceHeight = 0.0;
			destWidth = 0.0;
			destHeight = 0.0;
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
					if ( !in.getline ( filenameBuffer, 1000 ) )
						throw runtime_error ( "failed reading line from input file" );

					std::string ts ( filenameBuffer, in.gcount() );
					if ( ts.size() > 4 ) // at least ".pdf" because just test if ts is empty doesn't work.
					{
						multiSource.push_back ( ts );
						std::cerr << "Appending "<< ts <<" to source" << endl;
					}
				}
				while ( !in.eof() );
				in.close();
				delete [] filenameBuffer;
			}
			std::cerr<< ++dbg <<std::endl;

			if (multiSource.empty())
				throw runtime_error( "No recognized source given" );

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
                        std::cerr << "Unable to create Document: " << PdfError::ErrorMessage( e.GetError() ) << std::endl;
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
                PoDoFo::PdfPage* pFirstPage = sourceDoc->GetPage ( 0 );
                if ( NULL == pFirstPage ) // Fixes CVE-2019-9199 (issue #40)
                {
                    PODOFO_RAISE_ERROR_INFO( ePdfError_PageNotFound, "First page (0) of source document not found" );
                }
                PoDoFo::PdfRect rect ( pFirstPage->GetMediaBox() );
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

		PdfObject* PdfTranslator::migrateResource ( const PdfObject * obj )
		{
// 			std::cerr<<"PdfTranslator::migrateResource"<<std::endl;
			PdfObject *ret ( 0 );

			if ( !obj )
				PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "migrateResource called"
                                         " with NULL object" );

			if ( obj->IsDictionary() )
			{
				if ( obj->Reference().IsIndirect() )
				{
					ret = targetDoc->GetObjects().CreateObject ( *obj );
				}
				else
				{
					ret = new PdfObject( *obj );
				}
				TKeyMap resmap = obj->GetDictionary().GetKeys();
				for ( TCIKeyMap itres = resmap.begin(); itres != resmap.end(); ++itres )
				{
					PdfObject *o = itres->second;
					std::pair<std::set<PdfObject*>::iterator,bool> res = setMigrationPending.insert( o );
					if (!res.second)
					{
						std::ostringstream oss;
						oss << "Cycle detected: Object with ref " << o->Reference().ToString()
							<< " is already pending migration to the target.\n";
						PdfError::LogMessage( eLogSeverity_Warning, oss.str().c_str() );
						continue;
					}
					PdfObject *migrated = migrateResource ( o );
					if (NULL != migrated)
					{
						ret->GetDictionary().AddKey ( itres->first, migrated );
						if ( !(migrated->Reference().IsIndirect()) )
						{
							delete migrated;
						}
					}
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
					if ( NULL == co )
						continue;
					narray.push_back ( *co );

					if ( !(co->Reference().IsIndirect()) )
					{
						delete co;
					}
				}
				if ( obj->Reference().IsIndirect() )
				{
					ret = targetDoc->GetObjects().CreateObject ( narray );
				}
				else
				{
					ret = new PdfObject( narray );
				}
			}
			else if ( obj->IsReference() )
			{
				if ( migrateMap.find ( obj->GetReference().ToString() ) != migrateMap.end() )
				{
					std::ostringstream oss;
					oss << "Referenced object " << obj->GetReference().ToString()
					    << " already migrated." << std::endl;
					PdfError::DebugMessage( oss.str().c_str() );

					const PdfObject* const found = migrateMap[ obj->GetReference().ToString() ];
					return new PdfObject( found->Reference() );
				}

				PdfObject *to_migrate = sourceDoc->GetObjects().GetObject ( obj->GetReference() );

				std::pair<std::set<PdfObject*>::iterator, bool> res
						= setMigrationPending.insert( to_migrate );
				if (!res.second)
				{
					std::ostringstream oss;
					oss << "Cycle detected: Object with ref " << obj->GetReference().ToString()
						<< " is already pending migration to the target.\n";
					PdfError::LogMessage( eLogSeverity_Warning, oss.str().c_str() );	
					return NULL; // skip this migration
				}
				PdfObject * o ( migrateResource ( to_migrate ) );
				if ( NULL != o )
					ret  = new PdfObject ( o->Reference() );
				else
					return NULL; // avoid going through rest of method
			}
			else
			{
				ret = new PdfObject ( *obj );//targetDoc->GetObjects().CreateObject(*obj);
			}

			if ( obj->Reference().IsIndirect() )
			{
				migrateMap.insert ( std::pair<std::string, PdfObject*> ( obj->Reference().ToString(), ret ) );
			}

			return ret;
		}

		PdfObject* PdfTranslator::getInheritedResources ( PdfPage* page )
		{
// 			std::cerr<<"PdfTranslator::getInheritedResources"<<std::endl;
			PdfObject *res ( 0 );
			// mabri: resources are inherited as whole dict, not at all if the page has the dict
			// mabri: specified in PDF32000_2008.pdf section 7.7.3.4 Inheritance of Page Attributes
			// mabri: and in section 7.8.3 Resource Dictionaries
			const PdfObject *sourceRes = page->GetInheritedKey( PdfName ( "Resources" ) );
			if ( sourceRes )
			{
			    res = migrateResource( sourceRes );
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

				if (!page) // Fix issue #32
                {
                    std::ostringstream oss;
                    oss << "Page " << i << " (0-based) of " << pcount << " in source doc not found!";
                    PODOFO_RAISE_ERROR_INFO( ePdfError_PageNotFound, oss.str() );
                }
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
						PdfObject* migObj = migrateResource ( page->GetObject()->GetDictionary().GetKey ( keyname ) );
						if ( NULL == migObj )
							continue;
						xobj->GetObject()->GetDictionary().AddKey ( keyname, migObj ); 
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
							// mabri: ML post archive URL is https://sourceforge.net/p/podofo/mailman/message/24609746/
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

			// The following is necessary to avoid line 195 being detected as allocation having a memory leak
			// without changing other files than this one (thorough leak prevention shall be applied later).
			for (std::map<int, PdfObject*>::iterator it = resources.begin(); it != resources.end(); it++)
			{
				delete (*it).second;
			}
			resources.clear(); 
		}


	};
}; // end of namespace
