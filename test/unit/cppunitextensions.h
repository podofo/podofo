/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#include <cppunit/extensions/HelperMacros.h>
#include <new>
#include <exception>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ColorTest );

/** Added by RG to check if a suitable error message is returned
 * Asserts that the given expression throws an exception of the specified type. 
 * \ingroup Assertions
 * Example of usage:
 * \code
 *   std::vector<int> v;
 *  CPPUNIT_ASSERT_THROW( v.at( 50 ), std::out_of_range );
 * \endcode
 */
# define CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( expression, ExceptionType, errorType )              \
   CPPUNIT_ASSERT_THROW_MESSAGE_WITH_ERROR_TYPE( CPPUNIT_NS::AdditionalMessage(),       \
                                 expression,                            \
                                 ExceptionType,                         \
                                 errorType)

/** Added by RG to check if a suitable error message is returned
 * Asserts that the given expression throws an exception of the specified type, 
 * setting a user supplied message in case of failure. 
 * \ingroup Assertions
 * Example of usage:
 * \code
 *   std::vector<int> v;
 *  CPPUNIT_ASSERT_THROW_MESSAGE( "- std::vector<int> v;", v.at( 50 ), std::out_of_range );
 * \endcode
 */
# define CPPUNIT_ASSERT_THROW_MESSAGE_WITH_ERROR_TYPE( message, expression, ExceptionType, errorType )   \
   do {                                                                       \
      bool cpputCorrectExceptionThrown_ = false;                              \
      CPPUNIT_NS::Message cpputMsg_( "expected exception not thrown" );       \
      cpputMsg_.addDetail( message );                                         \
      cpputMsg_.addDetail( "Expected: "                                       \
                           CPPUNIT_GET_PARAMETER_STRING( ExceptionType ) );   \
                                                                              \
      try {                                                                   \
         expression;                                                          \
      } catch ( const ExceptionType &e) {                                     \
         if (e.GetError() == errorType)                                       \
         {                                                                    \
             cpputCorrectExceptionThrown_ = true;                             \
         }                                                                    \
         else                                                                 \
         {                                                                    \
             cpputMsg_.addDetail( "Error type mismatch. Actual: " #errorType ); \
             cpputMsg_.addDetail( std::string("What()  : ") + e.ErrorName(e.GetError()) );     \
         }                                                                    \
      } catch ( const std::exception &e) {                                    \
         cpputMsg_.addDetail( "Actual  : " +                                  \
                              CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e,             \
                                          "std::exception or derived") );     \
         cpputMsg_.addDetail( std::string("What()  : ") + e.what() );         \
      } catch ( ... ) {                                                       \
         cpputMsg_.addDetail( "Actual  : unknown.");                          \
      }                                                                       \
                                                                              \
      if ( cpputCorrectExceptionThrown_ )                                     \
         break;                                                               \
                                                                              \
      CPPUNIT_NS::Asserter::fail( cpputMsg_,                                  \
                                  CPPUNIT_SOURCELINE() );                     \
   } while ( false )

//GoogleTest compatible macros
#define ASSERT_TRUE(x) CPPUNIT_ASSERT(x)
#define ASSERT_FALSE(x) CPPUNIT_ASSERT(!(x))
#define EXPECT_TRUE(x) CPPUNIT_ASSERT(x)
#define EXPECT_FALSE(x) CPPUNIT_ASSERT(!(x))
#define EXPECT_EQ(expected, actual) CPPUNIT_ASSERT_EQUAL(expected, actual)
#define ASSERT_EQ(expected, actual) CPPUNIT_ASSERT_EQUAL(expected, actual)
#define EXPECT_NE(expected, actual) CPPUNIT_ASSERT(expected != actual)
#define ASSERT_NE(expected, actual) CPPUNIT_ASSERT(expected != actual)
#define EXPECT_DOUBLE_EQ(expected, actual, delta) CPPUNIT_ASSERT_DOUBLES_EQUAL(expected, actual, delta)
#define ASSERT_DOUBLE_EQ(expected, actual, delta) CPPUNIT_ASSERT_DOUBLES_EQUAL(expected, actual, delta)

