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

#ifndef OPENSSL_GLOBALS_H
#define OPENSSL_GLOBALS_H

typedef unsigned long ULONG;
typedef ULONG DWORD;

// -------------- OpenSSL Includes -----------------------
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/ocsp.h>
//#include <openssl/ts.h>
#include <openssl/pkcs12.h>

#include <string>

// Forward declaration.
namespace PKIBox
{
    namespace asn1
	{
		class CObjectID;
	}

	namespace x509
	{
		class CX509Certificate;
	}
}

extern int OIDtoNID(const PKIBox::asn1::CObjectID &OID);
extern std::string GetErrorDescription(DWORD dw);
extern std::string GetCertName(const PKIBox::x509::CX509Certificate &Cert);

extern time_t ASN1_GENERALIZEDTIME_get(ASN1_GENERALIZEDTIME *time);
extern time_t ASN1_UTCTIME_get(ASN1_TIME *s);

#define DECLARE_ASN1_DUP_FUNCTION(stname) \
	stname * stname##_dup(stname *x); 

#define IMPLEMENT_ASN1_DUP_FUNCTION_EX(stname) \
	stname * stname##_dup(stname *x) \
{ \
	return (stname *) ASN1_item_dup(ASN1_ITEM_rptr(stname), x); \
}

DECLARE_ASN1_DUP_FUNCTION(ASN1_BIT_STRING)
DECLARE_ASN1_DUP_FUNCTION(ASN1_OBJECT)
DECLARE_ASN1_DUP_FUNCTION(DIST_POINT)
DECLARE_ASN1_DUP_FUNCTION(ACCESS_DESCRIPTION)
DECLARE_ASN1_DUP_FUNCTION(X509_REVOKED)
DECLARE_ASN1_DUP_FUNCTION(X509_PUBKEY)
DECLARE_ASN1_DUP_FUNCTION(X509_SIG)


DECLARE_ASN1_DUP_FUNCTION(POLICYINFO)
DECLARE_ASN1_DUP_FUNCTION(POLICYQUALINFO)
DECLARE_ASN1_DUP_FUNCTION(USERNOTICE)
DECLARE_ASN1_DUP_FUNCTION(NOTICEREF)

#undef OCSP_CERTSTATUS_dup
DECLARE_ASN1_DUP_FUNCTION(OCSP_CERTSTATUS)
DECLARE_ASN1_DUP_FUNCTION(OCSP_REVOKEDINFO)
DECLARE_ASN1_DUP_FUNCTION(OCSP_ONEREQ)
DECLARE_ASN1_DUP_FUNCTION(OCSP_REQUEST)
DECLARE_ASN1_DUP_FUNCTION(OCSP_SINGLERESP)
DECLARE_ASN1_DUP_FUNCTION(OCSP_BASICRESP)
DECLARE_ASN1_DUP_FUNCTION(OCSP_RESPONSE)

DECLARE_ASN1_DUP_FUNCTION(PKCS7_SIGNED)
DECLARE_ASN1_DUP_FUNCTION(PKCS7_SIGNER_INFO)
DECLARE_ASN1_DUP_FUNCTION(PKCS7_ISSUER_AND_SERIAL)

DECLARE_ASN1_DUP_FUNCTION(PKCS7_ENVELOPE)
DECLARE_ASN1_DUP_FUNCTION(PKCS7_RECIP_INFO)
DECLARE_ASN1_DUP_FUNCTION(PKCS7_ENC_CONTENT)

DECLARE_ASN1_DUP_FUNCTION(PKCS7_SIGN_ENVELOPE)

DECLARE_ASN1_DUP_FUNCTION(PKCS8_PRIV_KEY_INFO)
DECLARE_ASN1_DUP_FUNCTION(PKCS12_SAFEBAG)
DECLARE_ASN1_DUP_FUNCTION(PKCS12_MAC_DATA)
DECLARE_ASN1_DUP_FUNCTION(PKCS12)

#define ASN1_TYPE_dup(asnt) (ASN1_TYPE *)ASN1_dup( (i2d_of_void *)i2d_ASN1_TYPE,\
	(d2i_of_void *)d2i_ASN1_TYPE, (char *)(asnt))

#endif // !OPENSSL_GLOBALS_H

