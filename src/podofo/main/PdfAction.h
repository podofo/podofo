/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ACTION_H
#define PDF_ACTION_H

#include "PdfDeclarations.h"

#include "PdfElement.h"

namespace PoDoFo {

class PdfObject;
class PdfString;
class PdfStreamedDocument;
class PdfIndirectObjectList;

/** The type of the action.
 *  PDF supports different action types, each of
 *  them has different keys and propeties.
 *
 *  Not all action types listed here are supported yet.
 *
 *  Please make also sure that the action type you use is
 *  supported by the PDF version you are using.
 */
enum class PdfActionType
{
    Unknown = 0,
    GoTo,
    GoToR,
    GoToE,
    Launch,
    Thread,
    URI,
    Sound,
    Movie,
    Hide,
    Named,
    SubmitForm,
    ResetForm,
    ImportData,
    JavaScript,
    SetOCGState,
    Rendition,
    Trans,
    GoTo3DView,
    RichMediaExecute,
};

/** An action that can be performed in a PDF document
 */
class PODOFO_API PdfAction final : public PdfDictionaryElement
{
    friend class PdfAnnotation;

public:
    /** Create a new PdfAction object
     *  \param eAction type of this action
     *  \param parent parent of this action
     */
    PdfAction(PdfDocument& doc, PdfActionType action);

    /** Create a PdfAction object from an existing
     *  PdfObject
     */
    PdfAction(PdfObject& obj);

    /** Set the URI of an PdfActionType::URI
     *  \param sUri must be a correct URI as PdfString
     */
    void SetURI(const PdfString& uri);

    /** Get the URI of an PdfActionType::URI
     *  \returns an URI
     */
    PdfString GetURI() const;

    /**
     *  \returns true if this action has an URI
     */
    bool HasURI() const;

    void SetScript(const PdfString& script);

    PdfString GetScript() const;

    bool HasScript() const;

    /** Get the type of this action
     *  \returns the type of this action
     */
    inline PdfActionType GetType() const { return m_Type; }

    /** Adds this action to an dictionary.
     *  This method handles the all the complexities of making sure it's added correctly
     *
     *  If this action is empty. Nothing will be added.
     *
     *  \param dictionary the action will be added to this dictionary
     */
    void AddToDictionary(PdfDictionary& dictionary) const;

private:
    PdfAction(const PdfAction& rhs) = delete;

private:
    PdfActionType m_Type;
};

};

#endif // PDF_ACTION_H
