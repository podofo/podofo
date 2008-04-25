/***************************************************************************
 *   Copyright (C) 2008 by Hashim Saleem                                   *
 *   hashim.saleem@gmail.com                                               *
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
 ***************************************************************************/

#include "PKIBox.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/pkcs12.h>

//--------- lib p11 includes ---------------
//#include <libp11.h>

//------- Entropy ( For seeding OpenSSL pseudo random number generator ) --------
//#include <egads.h>

#ifdef WIN32
#include <windows.h>
#endif 

#include "Exception.h"
#include "InvalidArgumentException.h"

using namespace std;

//namespace PKIBox
//{
	//---------------------------------------------------------------------------------------
	// Function name	: 
	// Description	    : 
	// Return type		: 
	// Argument         : 
	//---------------------------------------------------------------------------------------
	bool PKIBox::Initialize()
	{
//#ifdef WIN32
//		WORD wVersionRequested = 0;
//		WSADATA wsaData = {0};
//
//		wVersionRequested = MAKEWORD( 2, 2 );
//
//		int iRet = WSAStartup( wVersionRequested, &wsaData );
//#endif

		::ERR_load_BIO_strings();
		::ERR_load_crypto_strings();
		::ERR_load_BN_strings();
		::ERR_load_EVP_strings();
		::ERR_load_RSA_strings();
		::ERR_load_DSA_strings();
		::ERR_load_PKCS12_strings();

		OpenSSL_add_all_algorithms();
		::OpenSSL_add_all_digests();
		::SSL_library_init();
		::SSL_load_error_strings();

//		::ERR_load_PKCS11_strings();

		// ------------- seed the OpenSSL prng ---------------
		//int error;
		//prngctx_t ctx = {0};
		//egads_init(&ctx, NULL, NULL, &error);
		//if(error)
		//	return false;

		//int bytes = 255;
		//char *buf = (char *)malloc(bytes);
		//memset(buf, 0, bytes);
		//egads_entropy(&ctx, buf, bytes, &error);
		//if(!error)
			//RAND_seed(buf, bytes);
		RAND_seed(")1@(%NWPXf43YZmaj6 $:}.?", strlen(")1@(%NWPXf43YZmaj6 $:}.?") );
		//free(buf);

		//egads_destroy(&ctx);
		// ------------- seed the OpenSSL prng ---------------

		return true;
	}


	//---------------------------------------------------------------------------------------
	// Function name	: 
	// Description	    : 
	// Return type		: 
	// Argument         : 
	//---------------------------------------------------------------------------------------
	bool PKIBox::Uninitialize()
	{
		::EVP_cleanup();
		::ERR_free_strings();
		::CRYPTO_cleanup_all_ex_data();

//#ifdef WIN32
//		WSACleanup();
//#endif
		return true;
	}

//}


