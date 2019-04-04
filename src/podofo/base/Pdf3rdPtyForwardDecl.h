#ifndef PDF_FT_FORWARD_DECL
#define PDF_FT_FORWARD_DECL

/**
 * \page Pdf3rdPartyForwardDecl.h
 *
 * Forward declare some types that we use in our public API but don't want to
 * include the headers for directly. We can't do a nice simple forward
 * declaration because most of these libraries have typedefs everywhere.
 *
 * We don't want to include things like freetype directly in our public headers
 * because:
 *
 *  - They dump a huge amount of cruft into the top level namespace
 *
 *  - Programs that haven't gone through the apallingly convoluted process required
 *    to add freetype's header path can't include podofo's headers even if they have no
 *    intention of using any freetype-related font features.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Provide access to FT_Library
struct FT_LibraryRec_;
typedef struct FT_LibraryRec_  *FT_Library;

// Provide access to FT_Face
struct FT_FaceRec_;
typedef struct FT_FaceRec_*  FT_Face;

#if defined(PODOFO_HAVE_FONTCONFIG)
// Fontconfig
struct _FcConfig;
typedef struct _FcConfig    FcConfig;
#endif

#ifdef __cplusplus
}; // end extern "C"
#endif

// end PDF_FT_FORWARD_DECL
#endif
