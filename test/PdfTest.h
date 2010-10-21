
#include <podofo.h>

/* Common defines needed in all tests */
#define TEST_SAFE_OP( x ) try {  x; } catch( PdfError & e ) { \
                       e.AddToCallstack( __FILE__, __LINE__, NULL ); \
                       e.PrintErrorMsg();\
                       return e.GetError();\
                     }


#define TEST_SAFE_OP_IGNORE( x ) try {  x; } catch( PdfError & e ) { \
                       e.AddToCallstack( __FILE__, __LINE__, NULL ); \
                       e.PrintErrorMsg();\
                     }
