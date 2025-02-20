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

class PdfDocument;

/** The type of the action.
 *  PDF supports different action types, each of
 *  them has different keys and properties.
 *
 *  Not all action types listed here are supported yet.
 *
 *  Please make also sure that the action type you use is
 *  supported by the PDF version you are using.
 */
enum class PdfActionType : uint8_t
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
class PODOFO_API PdfAction : public PdfDictionaryElement
{
    friend class PdfDocument;
    friend class PdfAnnotationActionBase;
    friend class PdfOutlineItem;
    friend class PdfActionGoTo;
    friend class PdfActionGoToR;
    friend class PdfActionGoToE;
    friend class PdfActionLaunch;
    friend class PdfActionThread;
    friend class PdfActionURI;
    friend class PdfActionSound;
    friend class PdfActionMovie;
    friend class PdfActionHide;
    friend class PdfActionNamed;
    friend class PdfActionSubmitForm;
    friend class PdfActionResetForm;
    friend class PdfActionImportData;
    friend class PdfActionJavaScript;
    friend class PdfActionSetOCGState;
    friend class PdfActionRendition;
    friend class PdfActionTrans;
    friend class PdfActionGoTo3DView;
    friend class PdfActionRichMediaExecute;

private:
    PdfAction(PdfDocument& doc, PdfActionType action);

    PdfAction(PdfObject& obj, PdfActionType type);

protected:
    PdfAction(const PdfAction&) = default;

public:
    /** Get the type of this action
     *  \returns the type of this action
     */
    inline PdfActionType GetType() const { return m_Type; }

    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfAction>& action);

private:
    static std::unique_ptr<PdfAction> Create(PdfDocument& doc, PdfActionType type);

    static std::unique_ptr<PdfAction> Create(const PdfAction& action);

    template <typename TAction>
    static constexpr PdfActionType GetActionType();

    /** Adds this action to an dictionary.
     *  This method handles the all the complexities of making sure it's added correctly
     *
     *  If this action is empty. Nothing will be added.
     *
     *  \param dictionary the action will be added to this dictionary
     */
    void AddToDictionary(PdfDictionary& dictionary) const;

private:
    PdfActionType m_Type;
};


class PODOFO_API PdfActionGoTo final : public PdfAction
{
    friend class PdfAction;

    PdfActionGoTo(PdfDocument& doc);

    PdfActionGoTo(PdfObject& obj);

    PdfActionGoTo(const PdfActionGoTo&) = default;
public:
};

class PODOFO_API PdfActionGoToR final : public PdfAction
{
    friend class PdfAction;

    PdfActionGoToR(PdfDocument& doc);

    PdfActionGoToR(PdfObject& obj);

    PdfActionGoToR(const PdfActionGoToR&) = default;
public:
};

class PODOFO_API PdfActionGoToE final : public PdfAction
{
    friend class PdfAction;

    PdfActionGoToE(PdfDocument& doc);

    PdfActionGoToE(PdfObject& obj);

    PdfActionGoToE(const PdfActionGoToE&) = default;
public:
};

class PODOFO_API PdfActionLaunch final : public PdfAction
{
    friend class PdfAction;

    PdfActionLaunch(PdfDocument& doc);

    PdfActionLaunch(PdfObject& obj);

    PdfActionLaunch(const PdfActionLaunch&) = default;
public:
};

class PODOFO_API PdfActionThread final : public PdfAction
{
    friend class PdfAction;

    PdfActionThread(PdfDocument& doc);

    PdfActionThread(PdfObject& obj);

    PdfActionThread(const PdfActionThread&) = default;
public:
};

class PODOFO_API PdfActionURI final : public PdfAction
{
    friend class PdfAction;

    PdfActionURI(PdfDocument& doc);

    PdfActionURI(PdfObject& obj);

    PdfActionURI(const PdfActionURI&) = default;

public:
    /** Set the URI of an PdfActionType::URI
     *  \param sUri must be a correct URI as PdfString
     */
    void SetURI(nullable<const PdfString&> uri);

    /** Get the URI of an PdfActionType::URI
     *  \returns an URI
     */
    nullable<const PdfString&> GetURI() const;
};

class PODOFO_API PdfActionSound final : public PdfAction
{
    friend class PdfAction;

    PdfActionSound(PdfDocument& doc);

    PdfActionSound(PdfObject& obj);

    PdfActionSound(const PdfActionSound&) = default;
public:
};

class PODOFO_API PdfActionMovie final : public PdfAction
{
    friend class PdfAction;

    PdfActionMovie(PdfDocument& doc);

    PdfActionMovie(PdfObject& obj);

    PdfActionMovie(const PdfActionMovie&) = default;
public:
};

class PODOFO_API PdfActionHide final : public PdfAction
{
    friend class PdfAction;

    PdfActionHide(PdfDocument& doc);

    PdfActionHide(PdfObject& obj);

    PdfActionHide(const PdfActionHide&) = default;
public:
};

class PODOFO_API PdfActionNamed final : public PdfAction
{
    friend class PdfAction;

    PdfActionNamed(PdfDocument& doc);

    PdfActionNamed(PdfObject& obj);

    PdfActionNamed(const PdfActionNamed&) = default;
public:
};

class PODOFO_API PdfActionSubmitForm final : public PdfAction
{
    friend class PdfAction;

    PdfActionSubmitForm(PdfDocument& doc);

    PdfActionSubmitForm(PdfObject& obj);

    PdfActionSubmitForm(const PdfActionSubmitForm&) = default;
public:
};

class PODOFO_API PdfActionResetForm final : public PdfAction
{
    friend class PdfAction;

    PdfActionResetForm(PdfDocument& doc);

    PdfActionResetForm(PdfObject& obj);

    PdfActionResetForm(const PdfActionResetForm&) = default;
public:
};

class PODOFO_API PdfActionImportData final : public PdfAction
{
    friend class PdfAction;

    PdfActionImportData(PdfDocument& doc);

    PdfActionImportData(PdfObject& obj);

    PdfActionImportData(const PdfActionImportData&) = default;
public:
};

class PODOFO_API PdfActionJavaScript final : public PdfAction
{
    friend class PdfAction;

    PdfActionJavaScript(PdfDocument& doc);

    PdfActionJavaScript(PdfObject& obj);

    PdfActionJavaScript(const PdfActionJavaScript&) = default;
public:
    void SetScript(nullable<const PdfString&> script);

    nullable<const PdfString&> GetScript() const;
};

class PODOFO_API PdfActionSetOCGState final : public PdfAction
{
    friend class PdfAction;

    PdfActionSetOCGState(PdfDocument& doc);

    PdfActionSetOCGState(PdfObject& obj);

    PdfActionSetOCGState(const PdfActionSetOCGState&) = default;
public:
};

class PODOFO_API PdfActionRendition final : public PdfAction
{
    friend class PdfAction;

    PdfActionRendition(PdfDocument& doc);

    PdfActionRendition(PdfObject& obj);

    PdfActionRendition(const PdfActionRendition&) = default;
public:
};

class PODOFO_API PdfActionTrans : public PdfAction
{
    friend class PdfAction;

    PdfActionTrans(PdfDocument& doc);

    PdfActionTrans(PdfObject& obj);

    PdfActionTrans(const PdfActionTrans&) = default;
public:
};

class PODOFO_API PdfActionGoTo3DView final : public PdfAction
{
    friend class PdfAction;

    PdfActionGoTo3DView(PdfDocument& doc);

    PdfActionGoTo3DView(PdfObject& obj);

    PdfActionGoTo3DView(const PdfActionGoTo3DView&) = default;
public:
};

class PODOFO_API PdfActionRichMediaExecute final : public PdfAction
{
    friend class PdfAction;

    PdfActionRichMediaExecute(PdfDocument& doc);

    PdfActionRichMediaExecute(PdfObject& obj);

    PdfActionRichMediaExecute(const PdfActionRichMediaExecute&) = default;
public:
};

template<typename TAction>
constexpr PdfActionType PdfAction::GetActionType()
{
    if (std::is_same_v<TAction, PdfActionGoTo>)
        return PdfActionType::GoTo;
    else if (std::is_same_v<TAction, PdfActionGoToR>)
        return PdfActionType::GoToR;
    else if (std::is_same_v<TAction, PdfActionGoToE>)
        return PdfActionType::GoToE;
    else if (std::is_same_v<TAction, PdfActionLaunch>)
        return PdfActionType::Launch;
    else if (std::is_same_v<TAction, PdfActionThread>)
        return PdfActionType::Thread;
    else if (std::is_same_v<TAction, PdfActionURI>)
        return PdfActionType::URI;
    else if (std::is_same_v<TAction, PdfActionSound>)
        return PdfActionType::Sound;
    else if (std::is_same_v<TAction, PdfActionMovie>)
        return PdfActionType::Movie;
    else if (std::is_same_v<TAction, PdfActionHide>)
        return PdfActionType::Hide;
    else if (std::is_same_v<TAction, PdfActionNamed>)
        return PdfActionType::Named;
    else if (std::is_same_v<TAction, PdfActionSubmitForm>)
        return PdfActionType::SubmitForm;
    else if (std::is_same_v<TAction, PdfActionResetForm>)
        return PdfActionType::ResetForm;
    else if (std::is_same_v<TAction, PdfActionImportData>)
        return PdfActionType::ImportData;
    else if (std::is_same_v<TAction, PdfActionJavaScript>)
        return PdfActionType::JavaScript;
    else if (std::is_same_v<TAction, PdfActionSetOCGState>)
        return PdfActionType::SetOCGState;
    else if (std::is_same_v<TAction, PdfActionRendition>)
        return PdfActionType::Rendition;
    else if (std::is_same_v<TAction, PdfActionTrans>)
        return PdfActionType::Trans;
    else if (std::is_same_v<TAction, PdfActionGoTo3DView>)
        return PdfActionType::GoTo3DView;
    else if (std::is_same_v<TAction, PdfActionRichMediaExecute>)
        return PdfActionType::RichMediaExecute;
    else
        return PdfActionType::Unknown;
}

};

#endif // PDF_ACTION_H
