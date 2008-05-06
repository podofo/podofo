
#ifndef PKIBOX_ASN1_OIDS_H
#define PKIBOX_ASN1_OIDS_H

#include "ObjectID.h"

namespace PKIBox
{
	namespace asn1
	{
		//! This class groups the predefined (well known) objects identifiers.
		class OIDs  
		{
		public:

			// Distinguished Name

			//! ObjectID for the X.500 attribute type commonName (shortName CN). 
			static const ObjectID commonName;					// 2.5.4.3

			//! ObjectID for the X.500 attribute type surname (shortName SN). 
			static const ObjectID surname;					// 2.5.4.4

			//! ObjectID for the X.500 attribute type serialNumber. 
			static const ObjectID serialNumber;					// 2.5.4.5

			//! ObjectID for the X.500 attribute type countryName (shortName C). 
			static const ObjectID countryName;					// 2.5.4.6

			//! ObjectID for the X.500 attribute type localityName (shortName L). 
			static const ObjectID localityName;				// 2.5.4.7

			//! ObjectID for the X.500 attribute type stateOrProvinceName (shortName ST). 
			static const ObjectID stateOrProvinceName;			// 2.5.4.8

			//! ObjectID for the X.500 attribute type streetAddress. 
			static const ObjectID streetAddress;			// 2.5.4.9

			//! ObjectID for the X.500 attribute type organizationName (shortName O). 
			static const ObjectID organizationName;			// 2.5.4.10

			//! ObjectID for the X.500 attribute type organizationalUnitName (shortName OU). 
			static const ObjectID organizationalUnitName;		// 2.5.4.11

			// X.509 Extensions

			//! ObjectID for the X.500 object subjectKeyIdentifier. 
			static const ObjectID id_ce_subjectKeyIdentifier;		// 2.5.29.14

			//! ObjectID for the X.500 object keyUsage. 
			static const ObjectID id_ce_keyUsage;					// 2.5.29.15

			//static const ObjectID id_ce_privateKeyUsagePeriod;	
			//! ObjectID for the X.500 object subjectAltName. 
			static const ObjectID id_ce_subjectAltName;			// 2.5.29.17

			//! ObjectID for the X.500 object issuerAltName. 
			static const ObjectID id_ce_issuerAltName;				// 2.5.29.18

			//! ObjectID for the X.500 object basicConstraints. 
			static const ObjectID id_ce_basicConstraints;			// 2.5.29.19

			//! ObjectID for the X.500 object certificatePolicies. 
			static const ObjectID id_ce_certificatePolicies;		// 2.5.29.32

			//! ObjectID for the X.500 object anyPolicy. 
			static const ObjectID id_ce_anyPolicy;					// 2.5.29.32.0

			//static const ObjectID id_ce_policyMappings;

			//! ObjectID for the X.500 object authorityKeyIdentifier. 
			static const ObjectID id_ce_authorityKeyIdentifier;	// 2.5.29.35

			//! ObjectID for the X.500 object extKeyUsage. 
			static const ObjectID id_ce_extKeyUsage;				// 2.5.29.37

			//static const ObjectID id_ce_subjectDirectoryAttributes;
			//static const ObjectID id_ce_policyConstraints;
			//static const ObjectID id_ce_nameConstraints;

			//! ObjectID for the X.500 object cRLDistributionPoints. 
			static const ObjectID id_ce_cRLDistributionPoints;		// 2.5.29.31

			//! ObjectID for the X.500 object issuingDistributionPoint. 
			static const ObjectID id_ce_issuingDistributionPoint;	// 2.5.29.28

			//! ObjectID for the X.500 object authorityInfoAccess. 
			static const ObjectID id_pe_authorityInfoAccess;		// 1.3.6.1.5.5.7.1.1

			// Certificate Policy Extension

			//! ObjectID for the X.500 object cps. 
			static const ObjectID id_qt_cps;						// 1.3.6.1.5.5.7.2.1

			//! ObjectID for the X.500 object unotice. 
			static const ObjectID id_qt_unotice;					// 1.3.6.1.5.5.7.2.2

			// CRL
			//! ObjectID for the X.500 object cRLNumber. 
			static const ObjectID id_ce_cRLNumber;				// 2.5.29.20

			//! ObjectID for the X.500 object cRLReasons. 
			static const ObjectID id_ce_cRLReasons;				// 2.5.29.21

			//! ObjectID for the X.500 object holdInstructionCode. 
			static const ObjectID id_ce_holdInstructionCode;	// 2.5.29.23

			//! ObjectID for the X.500 object invalidityDate. 
			static const ObjectID id_ce_invalidityDate;			// 2.5.29.24

			//! ObjectID for the X.500 object deltaCRLIndicator. 
			static const ObjectID id_ce_deltaCRLIndicator;		// 2.5.29.27

			// OCSP

			//! ObjectID for the OCSP object basic. 
			static const ObjectID id_pkix_ocsp_basic;				// 1.3.6.1.5.5.7.48.1.1

			//! ObjectID for the OCSP object nonce. 
			static const ObjectID id_pkix_ocsp_nonce;				// 1.3.6.1.5.5.7.48.1.2

			//! ObjectID for the OCSP object response. 
			static const ObjectID id_pkix_ocsp_response;			// 1.3.6.1.5.5.7.48.1.3

			//! ObjectID for the OCSP object crl. 
			static const ObjectID id_pkix_ocsp_crl;				// 1.3.6.1.5.5.7.48.1.4

			//! ObjectID for the OCSP object nocheck. 
			static const ObjectID id_pkix_ocsp_nocheck;			// 1.3.6.1.5.5.7.48.1.5

			//! ObjectID for the OCSP object archive_cutoff. 
			static const ObjectID id_pkix_ocsp_archive_cutoff;		// 1.3.6.1.5.5.7.48.1.6

			//! ObjectID for the OCSP object service_locator. 
			static const ObjectID id_pkix_ocsp_service_locator;	// 1.3.6.1.5.5.7.48.1.7

			// Digest algorithms OIDs

			//! ObjectID for the Message Digest object md2. 
			static const ObjectID md2;							// 1.2.840.113549.2.2

			//! ObjectID for the Message Digest object md5. 
			static const ObjectID md5;							// 1.2.840.113549.2.5

			//! ObjectID for the Message Digest object sha1. 
			static const ObjectID sha1;						// 1.3.14.3.2.26 

			// Encryption algorithms OIDs

			//! ObjectID for the encryption algorithm dsa. 
			static const ObjectID  id_dsa;						// 1.2.840.10040.4.1

			//! ObjectID for the encryption algorithm rsaEncryption. 
			static const ObjectID  rsaEncryption;				// 1.2.840.113549.1.1.1

			//! ObjectID for the digest plus encryption algorithm sha1WithRSAEncryption. 
			static const ObjectID  sha1WithRSAEncryption;		// 1.2.840.113549.1.1.5

			//! ObjectID for encryption algorithm aes128
			static const ObjectID	aes128;						// 2.16.840.1.101.3.4.4

			// Extended KeyUsage Purposes 

			//! ObjectID for the extended key usage object serverAuth. 
			static const ObjectID id_kp_serverAuth;			// 1.3.6.1.5.5.7.3.1

			//! ObjectID for the extended key usage object clientAuth. 
			static const ObjectID id_kp_clientAuth;			// 1.3.6.1.5.5.7.3.2

			//! ObjectID for the extended key usage object codeSigning. 
			static const ObjectID id_kp_codeSigning;			// 1.3.6.1.5.5.7.3.3

			//! ObjectID for the extended key usage object emailProtection. 
			static const ObjectID id_kp_emailProtection;		// 1.3.6.1.5.5.7.3.4

			//! ObjectID for the extended key usage object ipsecEndSystem. 
			static const ObjectID id_kp_ipsecEndSystem;		// 1.3.6.1.5.5.7.3.5

			//! ObjectID for the extended key usage object ipsecTunnel. 
			static const ObjectID id_kp_ipsecTunnel;			// 1.3.6.1.5.5.7.3.6

			//! ObjectID for the extended key usage object ipsecUser. 
			static const ObjectID id_kp_ipsecUser;				// 1.3.6.1.5.5.7.3.7

			//! ObjectID for the extended key usage object timeStamping. 
			static const ObjectID id_kp_timeStamping;			// 1.3.6.1.5.5.7.3.8

			//! ObjectID for the extended key usage object OCSPSigning. 
			static const ObjectID id_kp_OCSPSigning;			// 1.3.6.1.5.5.7.3.9

			// Access methods
			//! ObjectID for the x.509 AccessDescription access method ocsp. 
			static const ObjectID id_ad_ocsp;					// 1.3.6.1.5.5.7.48.1 

			//! ObjectID for the x.509 AccessDescription access method caIssuers. 
			static const ObjectID id_ad_caIssuers;				// 1.3.6.1.5.5.7.48.2 

			// PKCS #7
			//! ObjectID for the PKCS#7 object data. 
			static const ObjectID data;						// 1.2.840.113549.1.7.1

			//! ObjectID for the PKCS#7 object signedData. 
			static const ObjectID signedData;					// 1.2.840.113549.1.7.2 

			//! ObjectID for the PKCS#7 object envelopedData. 
			static const ObjectID envelopedData;				// 1.2.840.113549.1.7.3 

			//! ObjectID for the PKCS#7 object signedAndEnvelopedData. 
			static const ObjectID signedAndEnvelopedData;		// 1.2.840.113549.1.7.4 

			//! ObjectID for the PKCS#7 object digestedData. 
			static const ObjectID digestedData;				// 1.2.840.113549.1.7.5 

			//! ObjectID for the PKCS#7 object encryptedData. 
			static const ObjectID encryptedData;				// 1.2.840.113549.1.7.6 

			// PKCS #9
			//! ObjectID for the PKCS#9 object emailAddress. 
			static const ObjectID id_emailAddress;						// 1.2.840.113549.1.9.1

			//! ObjectID for the PKCS#9 object unstructuredName. 
			static const ObjectID id_unstructuredName;					// 1.2.840.113549.1.9.2 

			//! ObjectID for the PKCS#9 object contentType. 
			static const ObjectID id_contentType;						// 1.2.840.113549.1.9.3

			//! ObjectID for the PKCS#9 object messageDigest. 
			static const ObjectID id_messageDigest;					// 1.2.840.113549.1.9.4 

			//! ObjectID for the PKCS#9 object signingTime. 
			static const ObjectID id_signingTime;						// 1.2.840.113549.1.9.5 

			//! ObjectID for the PKCS#9 object counterSignature. 
			static const ObjectID id_counterSignature;					// 1.2.840.113549.1.9.6

			//! ObjectID for the PKCS#9 object challengePassword. 
			static const ObjectID id_challengePassword;				// 1.2.840.113549.1.9.7 

			//! ObjectID for the PKCS#9 object unstructuredAddress. 
			static const ObjectID id_unstructuredAddress;				// 1.2.840.113549.1.9.8

			//! ObjectID for the PKCS#9 object extendedCertificateAttributes. 
			static const ObjectID id_extendedCertificateAttributes;	// 1.2.840.113549.1.9.9 

			//! ObjectID for the PKCS#9 object signingCertificate. 
			static const ObjectID id_aa_signingCertificate;		// 1.2.840.113549.1.9.16.2.12

			// PKCS #12
			//! ObjectID for the PKCS#12 object keyBag. 
			static const ObjectID keyBag;						// 1.2.840.113549.1.12.10.1.1

			//! ObjectID for the PKCS#12 object pkcs_8ShroudedKeyBag. 
			static const ObjectID pkcs_8ShroudedKeyBag;		// 1.2.840.113549.1.12.10.1.2

			//! ObjectID for the PKCS#12 object certBag. 
			static const ObjectID certBag;						// 1.2.840.113549.1.12.10.1.3

			//! ObjectID for the PKCS#12 object crlBag. 
			static const ObjectID crlBag;						// 1.2.840.113549.1.12.10.1.4

			//! ObjectID for the PKCS#12 object secretBag. 
			static const ObjectID secretBag;					// 1.2.840.113549.1.12.10.1.5

			//! ObjectID for the PKCS#12 object safeContentsBag. 
			static const ObjectID safeContentsBag;				// 1.2.840.113549.1.12.10.1.6
		};
	}
}

#endif // !PKIBOX_ASN1_OIDS_H

