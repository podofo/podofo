
#ifndef PKIBOX_ASN1_GENERAL_NAME_H
#define PKIBOX_ASN1_GENERAL_NAME_H

typedef struct GENERAL_NAME_st GENERAL_NAME;

#include <string>

namespace PKIBox
{
	namespace x509
	{
		namespace extensions
		{
			class CSubjectAltName;
			class CIssuerAltName;
			class CCRLDistributionPoint;
			class CAccessDescription;
		}

		namespace attr
		{
			class CIssuerSerial;
			class CHolder;
			class CV1Form;
			class CV2Form;
		}
	}

	namespace ocsp
	{
		class COCSPRequest;
	}

	namespace tsa
	{
		class CTSTInfo;
	}

	namespace cms
	{
		namespace ess
		{
			class CESSIssuerSerial;
		}
	}

	namespace asn1
	{
		// Forward declarations.
		class DistinguishedName;

		//! This class implements the ASN1. type "GeneralName".

		/*! 
		A GeneralName may be of one of the following types: 

		otherName <br>
		rfc822Name <br> 
		dNSName <br>
		x400Address <br>
		directoryName <br>
		ediPartyName <br>
		uniformResourceIdentifier <br>
		iPAddress <br>
		registeredID <br>

		The ASN.1 definition of GeneralName is: 

		GeneralName ::= CHOICE {<br>
		otherName                       [0]     OtherName,<br>
		rfc822Name                      [1]     IA5String,<br>
		dNSName                         [2]     IA5String,<br>
		x400Address                     [3]     ORAddress,<br>
		directoryName                   [4]     Name,<br>
		ediPartyName                    [5]     EDIPartyName,<br>
		uniformResourceIdentifier       [6]     IA5String,<br>
		iPAddress                       [7]     OCTET STRING,<br>
		registeredID                    [8]     OBJECT IDENTIFIER}<br>

		OtherName ::= SEQUENCE {<br>
		type-id    OBJECT IDENTIFIER,<br>
		value      [0] EXPLICIT ANY DEFINED BY type-id }<br>

		EDIPartyName ::= SEQUENCE {<br>
		nameAssigner            [0]     DirectoryString OPTIONAL,<br>
		partyName               [1]     DirectoryString }<br>

		DirectoryString ::= CHOICE {<br>
		teletexString           TeletexString (SIZE (1..maxSize),<br>
		printableString         PrintableString (SIZE (1..maxSize)),<br>
		universalString         UniversalString (SIZE (1..maxSize)),<br>
		utf8String              UTF8String (SIZE (1.. MAX)),<br>
		bmpString               BMPString (SIZE(1..maxSIZE))<br>
		}
		*/
		class GeneralName
		{
			friend class x509::extensions::CSubjectAltName;
			friend class x509::extensions::CIssuerAltName;
			friend class x509::extensions::CCRLDistributionPoint;
			friend class x509::extensions::CAccessDescription;
			friend class x509::attr::CIssuerSerial;
			friend class x509::attr::CHolder;
			friend class x509::attr::CV1Form;
			friend class x509::attr::CV2Form;
			friend class ocsp::COCSPRequest;
			friend class tsa::CTSTInfo;
			friend class cms::ess::CESSIssuerSerial;

		public:

			//! This enumeration represents the different GenerlName types.
			enum NameType
			{
				otherName,
				rfc822Name,
				dNSName,
				x400Address,
				directoryName,
				ediPartyName,
				uniformResourceIdentifier,
				iPAddress,
				registeredID,
				unInitialized
			};

			//! Default constructor. Initializes m_pName to NULL.
			GeneralName(void);

			virtual ~GeneralName(void);

			//! Copy constructor.
			/*!
				\param const GeneralName &rhs
			*/
			GeneralName(const GeneralName &rhs);

			//! Copy assignment operator.
			/*!
				\param const GeneralName &rhs
				\return GeneralName &
			*/
			GeneralName &operator=(const GeneralName &rhs);

			//! Returns the type of this general name.
			/*!
				\return NameType: the type of this general name
			*/
			NameType GetType() const;

			//! Returns RFC822 name.
			/*!
				\return std::string: value of GeneralName in RFC822 format
			*/
			std::string GetRFC822Name() const;

			//! Return the DNS Name.
			/*!
				\return std::string: value of GeneralName in DNS format
			*/
			std::string GetDNSName() const;

			//! Sets the DNS Name.
			/*!
				\param const std::string &DNS: DNS format string
			*/
			void SetDNSName(const std::string &DNS);

			//! Return the URL Name.
			/*!
				\return std::string: value of GeneralName in Uniform Resource Identifier
			*/
			std::string GetUniformResourceIdentifier() const;

			//! Sets the URL Name.
			/*!
				\param const std::string &sURL: Uniform Resource Identifier format string
			*/
			void SetUniformResourceIdentifier(const std::string &sURL);

			//! Return the Directory Name.
			/*!
				\return DistinguishedName: value of GeneralName in DirectoryName format
			*/
			DistinguishedName GetDirectoryName() const;

			//! Sets the Directory Name.
			/*!
				\param const DistinguishedName &dirName: DirecotryName format data
			*/
			void SetDirectoryName(const DistinguishedName &dirName);

			//! Equality operator.
			/*!
				\param const GeneralName &rhs
				\return bool
			*/
			bool operator==(const GeneralName &rhs) const;

			//! Inequality operator.
			/*!
				\param const GeneralName &rhs
				\return bool
			*/
			bool operator!=(const GeneralName &rhs) const;

		private:
			GENERAL_NAME *m_pName; // Pointer to underlying OpenSSL struct.
		};
	}
}

#endif //!PKIBOX_ASN1_GENERAL_NAME_H
