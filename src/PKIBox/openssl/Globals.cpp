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

#include "Globals.h"
#include "../Exception.h"
#include "../asn1/ObjectID.h"
//#include "../asn1/OIDs.h"
//#include "../asn1/DistinguishedName.h"
//#include "../asn1/GeneralName.h"
//#include "../x509/X509Extension.h"
//#include "../x509//extensions/SubjectAltName.h"
//#include "../x509/X509Certificate.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/conf.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>

// -------------- Windows Includes -----------------------
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#undef PKCS7_SIGNER_INFO

using namespace std;

IMPLEMENT_ASN1_DUP_FUNCTION_EX(ASN1_BIT_STRING);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(ASN1_OBJECT);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(DIST_POINT);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(ACCESS_DESCRIPTION);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(X509_REVOKED);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(X509_PUBKEY)
IMPLEMENT_ASN1_DUP_FUNCTION_EX(X509_SIG);

IMPLEMENT_ASN1_DUP_FUNCTION_EX(NOTICEREF);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(POLICYINFO);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(POLICYQUALINFO);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(USERNOTICE); 

IMPLEMENT_ASN1_DUP_FUNCTION_EX(OCSP_CERTSTATUS);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(OCSP_REVOKEDINFO);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(OCSP_ONEREQ);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(OCSP_REQUEST);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(OCSP_SINGLERESP);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(OCSP_BASICRESP);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(OCSP_RESPONSE);

IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS7_SIGNED);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS7_SIGNER_INFO);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS7_ISSUER_AND_SERIAL);

IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS7_ENVELOPE);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS7_RECIP_INFO);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS7_ENC_CONTENT);

IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS7_SIGN_ENVELOPE);

IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS8_PRIV_KEY_INFO);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS12_SAFEBAG);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS12_MAC_DATA);
IMPLEMENT_ASN1_DUP_FUNCTION_EX(PKCS12);


//---------------------------------------------------------------------------------------
// Function name	: 
// Description	    : 
// Return type		: 
// Argument         : 
//---------------------------------------------------------------------------------------
int OIDtoNID(const PKIBox::asn1::CObjectID &OID)
{
	using namespace PKIBox::asn1;

	/*if(OID == COIDs::commonName)
		return NID_commonName;

	else if(OID == COIDs::countryName)
		return NID_countryName;

	else if(OID == COIDs::localityName)
		return NID_localityName;

	else if(OID == COIDs::stateOrProvinceName)
		return NID_stateOrProvinceName;

	else if(OID == COIDs::organizationName)
		return NID_organizationName;

	else if(OID == COIDs::organizationalUnitName)
		return NID_organizationalUnitName;

	else if(OID == COIDs::id_ce_cRLDistributionPoints)
		return NID_crl_distribution_points;

	else if(OID == COIDs::id_ce_authorityKeyIdentifier)
		return NID_authority_key_identifier;

	else if(OID == COIDs::id_ce_keyUsage)
		return NID_key_usage;

	else if(OID == COIDs::id_ce_extKeyUsage)
		return NID_ext_key_usage;

	else if(OID == COIDs::id_pe_authorityInfoAccess)
		return NID_info_access;

	else if(OID == COIDs::id_ce_certificatePolicies)
		return NID_certificate_policies;

	else if(OID == COIDs::id_ce_subjectKeyIdentifier)
		return NID_subject_key_identifier;

	else if(OID == COIDs::id_ce_subjectAltName)
		return NID_subject_alt_name;

	else if(OID == COIDs::id_ce_basicConstraints)
		return NID_basic_constraints;

	else if(OID == COIDs::id_ce_invalidityDate)
		return NID_invalidity_date;

	else if(OID == COIDs::id_ce_cRLNumber)
		return NID_crl_number;

	else if(OID == COIDs::id_ce_cRLReasons)
		return NID_crl_reason;

	else if(OID == COIDs::id_ce_deltaCRLIndicator)
		return NID_delta_crl;

	else if(OID == COIDs::id_pkix_ocsp_nocheck)
		return NID_id_pkix_OCSP_noCheck;

	else if(OID == COIDs::id_pkix_ocsp_basic)
		return NID_id_pkix_OCSP_basic;

	else if(OID == COIDs::id_pkix_ocsp_nonce)
		return NID_id_pkix_OCSP_Nonce;

	else if(OID == COIDs::id_pkix_ocsp_response)
		return NID_id_pkix_OCSP_acceptableResponses;

	else if(OID == COIDs::id_pkix_ocsp_crl)
		return NID_id_pkix_OCSP_CrlID;

	else if(OID == COIDs::id_pkix_ocsp_archive_cutoff)
		return NID_id_pkix_OCSP_archiveCutoff;

	else if(OID == COIDs::id_pkix_ocsp_service_locator)
		return NID_id_pkix_OCSP_serviceLocator;

	else if(OID == COIDs::id_emailAddress)
		return NID_pkcs9_emailAddress;

	else if(OID == COIDs::id_unstructuredName)
		return NID_pkcs9_unstructuredName;

	else if(OID == COIDs::id_contentType)
		return NID_pkcs9_contentType;

	else if(OID == COIDs::id_messageDigest)
		return NID_pkcs9_messageDigest;

	else if(OID == COIDs::id_signingTime)
		return NID_pkcs9_signingTime;

	else if(OID == COIDs::id_counterSignature)
		return NID_pkcs9_countersignature;

	else if(OID == COIDs::id_challengePassword)
		return NID_pkcs9_challengePassword;

	else if(OID == COIDs::id_unstructuredAddress)
		return NID_pkcs9_unstructuredAddress;

	else if(OID == COIDs::id_extendedCertificateAttributes)
		return NID_pkcs9_extCertAttributes;

	else if(OID == COIDs::keyBag)
		return NID_keyBag;

	else if(OID == COIDs::pkcs_8ShroudedKeyBag)
		return NID_pkcs8ShroudedKeyBag;

	else if(OID == COIDs::certBag)
		return NID_certBag;

	else if(OID == COIDs::crlBag)
		return NID_crlBag;

	else if(OID == COIDs::secretBag)
		return NID_secretBag;

	else if(OID == COIDs::safeContentsBag)
		return NID_safeContentsBag;

	else*/
		return NID_undef;
}

//---------------------------------------------------------------------------------------
// Name			: 
// Description	:
// Return Type	:
//---------------------------------------------------------------------------------------
#ifdef _WIN32
string GetErrorDescription(DWORD dw)
{
	LPVOID lpMsgBuf;
	if (!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, 
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf, 0, NULL ))
	{
		// Handle the error.
		LocalFree( lpMsgBuf );
		return "";
	}

	string s( reinterpret_cast<char *>(lpMsgBuf));

	// Free the buffer.
	LocalFree( lpMsgBuf );

	return s;
}
#endif // _WIN32

//---------------------------------------------------------------------------------------
// Name			: 
// Description	: 
// Arguments	: 
// Return Type	: 
//---------------------------------------------------------------------------------------
string GetCertName(const PKIBox::x509::CX509Certificate &Cert)
{
	//try
	//{
	//	PKIBox::asn1::CDistinguishedName DN = Cert.GetSubjectDN();
	//	string szCertName = DN.GetRDN(PKIBox::asn1::COIDs::commonName);
	//	string::size_type RetVal = szCertName.find('/');
	//	if( RetVal != string::npos )
	//	{
	//		szCertName.clear();
	//	}
	//	if(szCertName.empty())
	//	{
	//		std::auto_ptr<PKIBox::x509::CX509Extension> pExt = Cert.GetExtension(PKIBox::asn1::COIDs::id_ce_subjectAltName);
	//		std::auto_ptr<PKIBox::x509::extensions::CSubjectAltName> pSubjectAltNameExt( dynamic_cast< PKIBox::x509::extensions::CSubjectAltName * >(pExt.release()));
	//		if(pSubjectAltNameExt.get())
	//		{
	//			vector<PKIBox::asn1::CGeneralName> vGeneralNames = pSubjectAltNameExt->GetGeneralNames();
	//			if(!vGeneralNames.empty())
	//			{
	//				if( PKIBox::asn1::CGeneralName::directoryName == vGeneralNames[0].GetType() )
	//				{
	//					szCertName = vGeneralNames[0].GetDirectoryName().GetRDN(PKIBox::asn1::COIDs::commonName);
	//					if(!szCertName.empty())
	//						return szCertName;
	//				}
	//			}
	//		}

	//		szCertName = DN.GetRDN(PKIBox::asn1::COIDs::organizationalUnitName);
	//		RetVal = szCertName.find('/');
	//		if( RetVal != string::npos )
	//		{
	//			szCertName.clear();
	//		}
	//		if(szCertName.empty())
	//		{
	//			szCertName = DN.GetRDN(PKIBox::asn1::COIDs::organizationName);
	//			RetVal = szCertName.find('/');
	//			if( RetVal != string::npos )
	//			{
	//				szCertName.clear();
	//			}
	//			if(szCertName.empty())
	//			{
	//				szCertName = DN.GetRDN(PKIBox::asn1::COIDs::localityName);
	//				RetVal = szCertName.find('/');
	//				if( RetVal != string::npos )
	//				{
	//					szCertName.clear();
	//				}
	//				if(szCertName.empty())
	//				{
	//					szCertName = DN.GetRDN(PKIBox::asn1::COIDs::stateOrProvinceName);
	//					RetVal = szCertName.find('/');
	//					if( RetVal != string::npos )
	//					{
	//						szCertName.clear();
	//					}
	//					if(szCertName.empty())
	//					{
	//						szCertName = DN.GetRDN(PKIBox::asn1::COIDs::countryName);
	//					}
	//				}
	//			}
	//		}
	//	}

	//	return szCertName;
	//}
	//catch (PKIBox::CException &)
	//{
	//	
	//}
	return "";
}

//---------------------------------------------------------------------------------------
// Function name	: 
// Description	    : 
// Return type		: 
// Argument         : 
//---------------------------------------------------------------------------------------
time_t ASN1_GENERALIZEDTIME_get(ASN1_GENERALIZEDTIME *time)
{
	char *v;
	int gmt = 0;
	int i;
	int y=0, M=0, d=0, h=0, m=0, s=0;
	char *f = NULL;
	int f_len = 0;
        time_t tim;
	tm t = {0};

	i=time->length;
	v=(char *)time->data;

	if (i < 12) 
		goto err;

	if (v[i-1] == 'Z') 
		gmt=1;

	for (i=0; i<12; i++)
		if ((v[i] > '9') || (v[i] < '0')) 
			goto err;

	y= (v[0]-'0')*1000+(v[1]-'0')*100 + (v[2]-'0')*10+(v[3]-'0');
	M= (v[4]-'0')*10+(v[5]-'0');

	if ((M > 12) || (M < 1)) 
		goto err;

	d= (v[6]-'0')*10+(v[7]-'0');
	h= (v[8]-'0')*10+(v[9]-'0');
	m=  (v[10]-'0')*10+(v[11]-'0');

	if (	(v[12] >= '0') && (v[12] <= '9') && (v[13] >= '0') && (v[13] <= '9'))
	{
		s=  (v[12]-'0')*10+(v[13]-'0');
		/* Check for fractions of seconds. */
		if (v[14] == '.')
		{
			int l = time->length;
			f = &v[14];	/* The decimal point. */
			f_len = 1;
			while (14 + f_len < l && f[f_len] >= '0' && f[f_len] <= '9')
				++f_len;
		}
	}

	t.tm_year = y - 1900 ;
	t.tm_mon = M - 1;
	t.tm_mday = d;
	t.tm_hour = h;
	t.tm_min = m;
	t.tm_sec = s;

	tim = mktime(&t); 
	tzset();
#ifdef _WIN32
	return tim - _timezone;
#else
	return tim - timezone;
#endif // _WIN32

err:
	return 0;
}



//---------------------------------------------------------------------------------------
// Function name	: 
// Description	    : 
// Return type		: 
// Argument         : 
//---------------------------------------------------------------------------------------
time_t ASN1_UTCTIME_get(ASN1_TIME *s)
{ 
	time_t lResult = 0; 

	char lBuffer[24]; 
	char * pBuffer = lBuffer; 


	size_t lTimeLength = s->length; 
	char * pString = (char *)s->data; 


	if (s->type == V_ASN1_UTCTIME) 
	{ 
		if ((lTimeLength < 11) || (lTimeLength > 17)) 
		{ 
			return 0; 
		} 


		memcpy(pBuffer, pString, 10); 
		pBuffer += 10; 
		pString += 10; 
	} 
	else 
	{ 
		if (lTimeLength < 13) 
		{ 
			return 0; 
		} 


		memcpy(pBuffer, pString, 12); 
		pBuffer += 12; 
		pString += 12; 
	} 


	if ((*pString == 'Z') || (*pString == '-') || (*pString == '+')) 
	{ 
		*(pBuffer++) = '0'; 
		*(pBuffer++) = '0'; 
	} 
	else 
	{ 
		*(pBuffer++) = *(pString++); 
		*(pBuffer++) = *(pString++); 
		// Skip any fractional seconds... 
		if (*pString == '.') 
		{ 
			pString++; 
			while ((*pString >= '0') && (*pString <= '9')) 
			{ 
				pString++; 
			} 
		} 
	} 


	*(pBuffer++) = 'Z'; 
	*(pBuffer++) = '\0'; 


	time_t lSecondsFromUCT; 
	if (*pString == 'Z') 
	{ 
		lSecondsFromUCT = 0; 
	} 
	else 
	{ 
		if ((*pString != '+') && (pString[5] != '-')) 
		{ 
			return 0; 
		} 


		lSecondsFromUCT = ((pString[1]-'0') * 10 + (pString[2]-'0')) * 60; 
		lSecondsFromUCT += (pString[3]-'0') * 10 + (pString[4]-'0'); 
		if (*pString == '-') 
		{ 
			lSecondsFromUCT = lSecondsFromUCT; 
		} 
	} 


	tm lTime; 
	lTime.tm_sec  = ((lBuffer[10] - '0') * 10) + (lBuffer[11] - '0'); 
	lTime.tm_min  = ((lBuffer[8] - '0') * 10) + (lBuffer[9] - '0'); 
	lTime.tm_hour = ((lBuffer[6] - '0') * 10) + (lBuffer[7] - '0'); 
	lTime.tm_mday = ((lBuffer[4] - '0') * 10) + (lBuffer[5] - '0'); 
	lTime.tm_mon  = (((lBuffer[2] - '0') * 10) + (lBuffer[3] - '0')) - 1; 
	lTime.tm_year = ((lBuffer[0] - '0') * 10) + (lBuffer[1] - '0'); 
	if (lTime.tm_year < 50) 
	{ 
		lTime.tm_year += 100; // RFC 2459 
	} 
	lTime.tm_wday = 0; 
	lTime.tm_yday = 0; 
	lTime.tm_isdst = 0;  // No DST adjustment requested 


	lResult = mktime(&lTime); 
	if ((time_t)-1 != lResult) 
	{ 
		if (0 != lTime.tm_isdst) 
		{ 
			lResult -= 3600;  // mktime may adjust for­ DST  (OS dependent) 
		} 
		lResult += lSecondsFromUCT; 
	} 
	else 
	{ 
		lResult = 0; 
	} 


	return lResult; 
} 




