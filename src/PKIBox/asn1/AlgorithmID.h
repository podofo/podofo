
#ifndef PKIBOX_ASN1_ALGORITHM_ID_H
#define PKIBOX_ASN1_ALGORITHM_ID_H

typedef struct X509_algor_st X509_ALGOR;

namespace PKIBox
{
	namespace utils
	{
		class ByteArray;
	}

	namespace x509
	{
		class CPublicKeyInfo;
		class CX509Certificate;
		class CX509CRL;
		namespace attr
		{
			class CObjectDigestInfo;
		}
	}

	namespace pkcs7
	{
		class CDigestInfo;
		class CSignerInfo;
		class CSignedData;
		class CRecipientInfo;
		class CEnvelopedData;
		class CEncryptedContentInfo;
		class CSignedAndEnvelopedData;
	}

	namespace pkcs8
	{
		class CPrivateKeyInfo;
		class CEncryptedPrivateKeyInfo;
	}

	namespace pkcs10
	{
		class CCertificateRequest;
	}

	namespace tsa
	{
		class CMessageImprint;
	}

	namespace cms
	{
		class CSignerInfo;
		class CSignedData;
	}

	namespace asn1
	{
		// Forward declarations.
		class ObjectID;

		//! This class implements the ASN.1 type "AlgorithmIdentifier". 
		/*! 
			An AlgorithmID object unequivocally identifies some specific algorithm by assigning a 
			particular ObjectID to it. An algorithmID optionally may include algorithm parameters.

			The ASN.1 definition of AlgorithmIdentifier is

			AlgorithmIdentifier  ::=  SEQUENCE  {<br>
				algorithm               OBJECT IDENTIFIER,<br>
				parameters              ANY DEFINED BY algorithm OPTIONAL  }<br>

			An AlgorithmID object may be, for instance, used for specifying the signature algorithm when signing a X509Certificate, e,g.: 

			CX509Certificate cert;
			...
			AlgorithmID algoID(OIDs::sha1);
			cert.sign(algoID, issuerPrivateKey);

		*/
		class AlgorithmID
		{
			friend class x509::CPublicKeyInfo;
			friend class x509::CX509Certificate;
			friend class x509::CX509CRL;
			friend class x509::attr::CObjectDigestInfo;
			friend class pkcs7::CSignedData;
			friend class pkcs7::CSignerInfo;
			friend class pkcs7::CDigestInfo;
			friend class pkcs7::CRecipientInfo;
			friend class pkcs7::CEncryptedContentInfo;
			friend class pkcs7::CEnvelopedData;
			friend class pkcs7::CSignedAndEnvelopedData;
			friend class pkcs8::CPrivateKeyInfo;
			friend class pkcs8::CEncryptedPrivateKeyInfo;
			friend class pkcs10::CCertificateRequest;
			friend class tsa::CMessageImprint;
			friend class cms::CSignerInfo;
			friend class cms::CSignedData;
		public:
			//! Default constructor. Initializes m_pAlgID to NULL.
			AlgorithmID(void);

			virtual ~AlgorithmID(void);

			//! Creates a new AlgorithmID from an ObjectID and algorithm parameters.
			/*!
				\param const ObjectID &algorithm: the ObjectID of the algorithm
				\param const utils::ByteArray &parameter: the algorithm parameters
			*/
			AlgorithmID(const ObjectID &algorithm, const utils::ByteArray &parameter);

			//! Creates a new AlgorithmID from an ObjectID. 
			/*!
				\param const ObjectID &Algorithm: the ObjectID of the algorithm
			*/
			explicit AlgorithmID(const ObjectID &Algorithm);

			//! Copy constructor.
			/*!
				\param const AlgorithmID &rhs
			*/
			AlgorithmID(const AlgorithmID &rhs);

			//! Copy assignment operator.
			/*!
				\param const AlgorithmID &rhs
				\return AlgorithmID &
			*/
			AlgorithmID &operator=(const AlgorithmID &rhs);

			//! Returns the AlgorithmIdentifier.
			/*!
				\return ObjectID: the ObjectID of the algorithm
			*/
			ObjectID GetAlgorithm() const;

			//! Sets the Algorithm object identifier
			/*!
				\param const ObjectID &obj: the ObjectID of the algorithm
			*/
			void SetAlgorithm(const ObjectID &obj);

			//! Returns the algorithm's parameters. Returns an empty array if they are not defined for this particular algorithm.
			/*!
				\return utils::ByteArray: the parameters of the algorithm
			*/
			utils::ByteArray GetParameters() const;

		private:
			X509_ALGOR	*m_pAlgID; // Pointer to underlying OpenSSL struct.
		};
	}
}


#endif // !PKIBOX_ASN1_ALGORITHM_ID_H

