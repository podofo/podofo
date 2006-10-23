#ifndef PODOFO_API_H_20061017
#define PODOFO_API_H_20061017

/*
 * This header provides a macro to handle correct symbol imports/exports
 * on platforms that require explicit instructions to make symbols public,
 * or differentiate between exported and imported symbols.
 * 
 * Win32 compilers use this information, and gcc4 can use it on *nix
 * to reduce the size of the export symbol table and get faster runtime
 * linking.
 *
 * All declarations of public API should be marked with the PODOFO_API macro.
 * Separate definitions need not be annotated, even in headers.
 *
 * Usage examples:
 *
 * class PODOFO_API PdfArray : public PdfDataType {
 *     ...
 * };
 *
 * bool PODOFO_API doThatThing(void);
 *
 * For an exception type that may be thrown across a DSO boundary, you must
 * use:
 *
 * class PODOFO_EXCEPTION_API(PODOFO_API) MyException
 * {
 *     ...
 * };
 *
 * PODOFO_LOCAL can be used on members of a class exported with PODOFO_API
 * to omit some members from the symbol table on supporting platforms. This
 * helps keep the exported API cleaner and the symbol table smaller.
 *
 * For information on the gcc visibility support see:
 *	http://gcc.gnu.org/wiki/Visibility
 *	http://people.redhat.com/drepper/dsohowto.pdf
 *
 */

/* Automatically defined by CMake when building a shared library */
#if defined(podofo_EXPORTS)
    #define COMPILING_SHARED_PODOFO
#endif

/* Sanity check - can't be both compiling and using shared podofo */
#if defined(COMPILING_SHARED_PODOFO) && defined(USING_SHARED_PODOFO)
    #error "Both COMPILING_SHARED_PODOFO and USING_SHARED_PODOFO defined!"
#endif

/*
 * Define COMPILING_SHARED_PODOFO when building the PoDoFo library as a
 * DLL. When building code that uses that DLL, define USING_SHARED_PODOFO.
 *
 * Building or linking to a static library does not require either
 * preprocessor symbol.
 */
#if defined(_WIN32)
    #if defined(COMPILING_SHARED_PODOFO)
        #define PODOFO_API __declspec(dllexport)
    #elif defined(USING_SHARED_PODOFO)
        #define PODOFO_API __declspec(dllimport)
    #else
        #define PODOFO_API
    #endif
    /* PODOFO_LOCAL doesn't mean anything on win32, it's to exclude
     * symbols from the export table with gcc4. */
    #define PODOFO_LOCAL
#else
    #if defined(HAVE_GCC_SYMBOL_VISIBILITY)
        /* Forces inclusion of a symbol in the symbol table, so
           software outside the current library can use it. */
        #define PODOFO_API __attribute__ ((visibility("default")))
        /* Within a section exported with SCRIBUS_API, forces a symbol to be
           private to the library / app. Good for private members. */
        #define PODOFO_LOCAL __attribute__ ((visibility("hidden")))
    #else
        #define PODOFO_API
        #define PODOFO_LOCAL
    #endif
#endif

/* Throwable classes must always be exported by all binaries when
 * using gcc. Marking exception classes with PODOFO_EXCEPTION_API
 * ensures this. */
#ifdef _WIN32
  #define PODOFO_EXCEPTION_API(api) api
#elif defined(HAVE_GCC_SYMBOL_VISIBILITY)
  #define PODOFO_EXCEPTION_API(api) PODOFO_API
#else
  #define PODOFO_EXCEPTION_API(api)
#endif

#endif
