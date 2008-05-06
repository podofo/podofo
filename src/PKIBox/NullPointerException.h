
#ifndef PKIBOX_NULL_POINTER_EXCEPTION_H
#define PKIBOX_NULL_POINTER_EXCEPTION_H

#include "Exception.h"

namespace PKIBox
{
	//! Thrown when a null pointer is passed to a method that requires a valid pointer to be passed.
	class NullPointerException : public Exception
	{
	public:
		//! Constructs a NullPointerException without a description message.
		NullPointerException(void);

		//! Constructs a NullPointerException from a description message.
		/*!
			\param const std::string &sErrorMsg: string containing error message
		*/
		explicit NullPointerException(const std::string &sErrorMsg);

		virtual ~NullPointerException(void);
	};
}

#endif // !PKIBOX_NULL_POINTER_EXCEPTION_H



