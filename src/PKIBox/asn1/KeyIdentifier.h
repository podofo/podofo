
#ifndef PKIBOX_ASN1_KEY_IDENTIFIER_H
#define PKIBOX_ASN1_KEY_IDENTIFIER_H

namespace PKIBox
{
	namespace asn1
	{
		//! This class represents the Base of all Key identifiers in CMS (i.e SubjectKeyIdentifier , certificate identifier etc). 
		class KeyIdentifier
		{
		public:
			//! Default Constructor
			KeyIdentifier();

			virtual ~KeyIdentifier();
		};
	}
}

#endif // !PKIBOX_ASN1_KEY_IDENTIFIER_H

