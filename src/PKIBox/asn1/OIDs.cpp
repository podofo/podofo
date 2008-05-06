
#include "OIDs.h"

namespace PKIBox
{
	namespace asn1
	{
		// Distinguished Name
		const ObjectID OIDs::commonName("2.5.4.3");
		const ObjectID OIDs::surname("2.5.4.4");
		const ObjectID OIDs::serialNumber("2.5.4.5");
		const ObjectID OIDs::countryName("2.5.4.6");
		const ObjectID OIDs::localityName("2.5.4.7");
		const ObjectID OIDs::stateOrProvinceName("2.5.4.8");
		const ObjectID OIDs::streetAddress("2.5.4.9");
		const ObjectID OIDs::organizationName("2.5.4.10");
		const ObjectID OIDs::organizationalUnitName("2.5.4.11");

		// X.509 Extensions
		const ObjectID OIDs::id_ce_subjectKeyIdentifier("2.5.29.14");
		const ObjectID OIDs::id_ce_keyUsage("2.5.29.15");
		//const ObjectID OIDs::id_ce_privateKeyUsagePeriod("");
		const ObjectID OIDs::id_ce_subjectAltName("2.5.29.17");
		const ObjectID OIDs::id_ce_issuerAltName("2.5.29.18");
		const ObjectID OIDs::id_ce_basicConstraints("2.5.29.19");
		const ObjectID OIDs::id_ce_certificatePolicies("2.5.29.32");
		const ObjectID OIDs::id_ce_anyPolicy("2.5.29.32.0");
		//const ObjectID OIDs::id_ce_policyMappings("");
		const ObjectID OIDs::id_ce_authorityKeyIdentifier("2.5.29.35");
		const ObjectID OIDs::id_ce_extKeyUsage("2.5.29.37");
		//const ObjectID OIDs::id_ce_subjectDirectoryAttributes("");
		//const ObjectID OIDs::id_ce_policyConstraints("");
		//const ObjectID OIDs::id_ce_nameConstraints("");
		const ObjectID OIDs::id_ce_cRLDistributionPoints("2.5.29.31");
		const ObjectID OIDs::id_ce_issuingDistributionPoint("2.5.29.28");
		const ObjectID OIDs::id_pe_authorityInfoAccess("1.3.6.1.5.5.7.1.1");

		// Certificate Policy Extension
		const ObjectID OIDs::id_qt_cps("1.3.6.1.5.5.7.2.1");
		const ObjectID OIDs::id_qt_unotice("1.3.6.1.5.5.7.2.2");

		// CRL
		const ObjectID OIDs::id_ce_cRLNumber("2.5.29.20");
		const ObjectID OIDs::id_ce_cRLReasons("2.5.29.21");
		const ObjectID OIDs::id_ce_holdInstructionCode("2.5.29.23");
		const ObjectID OIDs::id_ce_invalidityDate("2.5.29.24");
		const ObjectID OIDs::id_ce_deltaCRLIndicator("2.5.29.27");

		// OCSP
		const ObjectID OIDs::id_pkix_ocsp_basic("1.3.6.1.5.5.7.48.1.1");
		const ObjectID OIDs::id_pkix_ocsp_nonce("1.3.6.1.5.5.7.48.1.2");
		const ObjectID OIDs::id_pkix_ocsp_response("1.3.6.1.5.5.7.48.1.3");
		const ObjectID OIDs::id_pkix_ocsp_crl("1.3.6.1.5.5.7.48.1.4");
		const ObjectID OIDs::id_pkix_ocsp_nocheck("1.3.6.1.5.5.7.48.1.5");
		const ObjectID OIDs::id_pkix_ocsp_archive_cutoff("1.3.6.1.5.5.7.48.1.6");
		const ObjectID OIDs::id_pkix_ocsp_service_locator("1.3.6.1.5.5.7.48.1.7");

		// Digest algorithms OIDs
		const ObjectID OIDs::md2("1.2.840.113549.2.2");
		const ObjectID OIDs::md5("1.2.840.113549.2.5");
		const ObjectID OIDs::sha1("1.3.14.3.2.26");


		// Encryption algorithms OIDs
		const ObjectID OIDs::id_dsa("1.2.840.10040.4.1");		
		const ObjectID OIDs::rsaEncryption("1.2.840.113549.1.1.1");
		const ObjectID OIDs::aes128("2.16.840.1.101.3.4.4");

		// Encryption algorithms OIDs
		const ObjectID OIDs::sha1WithRSAEncryption("1.2.840.113549.1.1.5");

		// Extended KeyUsage Purposes 
		const ObjectID OIDs::id_kp_serverAuth("1.3.6.1.5.5.7.3.1");
		const ObjectID OIDs::id_kp_clientAuth("1.3.6.1.5.5.7.3.2");
		const ObjectID OIDs::id_kp_codeSigning("1.3.6.1.5.5.7.3.3");
		const ObjectID OIDs::id_kp_emailProtection("1.3.6.1.5.5.7.3.4");
		const ObjectID OIDs::id_kp_ipsecEndSystem("1.3.6.1.5.5.7.3.5");
		const ObjectID OIDs::id_kp_ipsecTunnel("1.3.6.1.5.5.7.3.6");
		const ObjectID OIDs::id_kp_ipsecUser("1.3.6.1.5.5.7.3.7");
		const ObjectID OIDs::id_kp_timeStamping("1.3.6.1.5.5.7.3.8");
		const ObjectID OIDs::id_kp_OCSPSigning("1.3.6.1.5.5.7.3.9");

		// Access methods
		const ObjectID OIDs::id_ad_ocsp("1.3.6.1.5.5.7.48.1");
		const ObjectID OIDs::id_ad_caIssuers("1.3.6.1.5.5.7.48.2");

		// PKCS #7
		const ObjectID OIDs::data("1.2.840.113549.1.7.1");
		const ObjectID OIDs::signedData("1.2.840.113549.1.7.2");
		const ObjectID OIDs::envelopedData("1.2.840.113549.1.7.3"); 
		const ObjectID OIDs::signedAndEnvelopedData("1.2.840.113549.1.7.4");
		const ObjectID OIDs::digestedData("1.2.840.113549.1.7.5");
		const ObjectID OIDs::encryptedData("1.2.840.113549.1.7.6");

		// PKCS #9
		const ObjectID OIDs::id_emailAddress("1.2.840.113549.1.9.1"); 
		const ObjectID OIDs::id_unstructuredName("1.2.840.113549.1.9.2");
		const ObjectID OIDs::id_contentType("1.2.840.113549.1.9.3");					 
		const ObjectID OIDs::id_messageDigest("1.2.840.113549.1.9.4");					  
		const ObjectID OIDs::id_signingTime("1.2.840.113549.1.9.5");					  
		const ObjectID OIDs::id_counterSignature("1.2.840.113549.1.9.6"); 
		const ObjectID OIDs::id_challengePassword("1.2.840.113549.1.9.7");				  
		const ObjectID OIDs::id_unstructuredAddress("1.2.840.113549.1.9.8");            
		const ObjectID OIDs::id_extendedCertificateAttributes("1.2.840.113549.1.9.9");	  

		// ESS
		const ObjectID OIDs::id_aa_signingCertificate("1.2.840.113549.1.9.16.2.12");

		// PKCS #12
		const ObjectID OIDs::keyBag("1.2.840.113549.1.12.10.1.1");
		const ObjectID OIDs::pkcs_8ShroudedKeyBag("1.2.840.113549.1.12.10.1.2");
		const ObjectID OIDs::certBag("1.2.840.113549.1.12.10.1.3");
		const ObjectID OIDs::crlBag("1.2.840.113549.1.12.10.1.4");
		const ObjectID OIDs::secretBag("1.2.840.113549.1.12.10.1.5");
		const ObjectID OIDs::safeContentsBag("1.2.840.113549.1.12.10.1.6");
	}
}

