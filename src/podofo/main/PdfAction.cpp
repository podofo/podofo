/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfAction.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

static const char* s_names[] = {
    nullptr,
    "GoTo",
    "GoToR",
    "GoToE",
    "Launch",
    "Thread",
    "URI",
    "Sound",
    "Movie",
    "Hide",
    "Named",
    "SubmitForm",
    "ResetForm",
    "ImportData",
    "JavaScript",
    "SetOCGState",
    "Rendition",
    "Trans",
    "GoTo3DView",
};

PdfAction::PdfAction(PdfDocument& doc, PdfActionType type)
    : PdfDictionaryElement(doc, "Action"_n), m_Type(type)
{
    PdfName typeName(utls::TypeNameForIndex((unsigned)type, s_names, (unsigned)std::size(s_names)));
    if (typeName.IsNull())
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    GetDictionary().AddKey("S"_n, typeName);
}

PdfAction::PdfAction(PdfObject& obj, PdfActionType type)
    : PdfDictionaryElement(obj), m_Type(type) { }

bool PdfAction::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfAction>& action)
{
    auto type = static_cast<PdfActionType>(utls::TypeNameToIndex(
        obj.GetDictionary().FindKeyAsSafe<PdfName>("S").GetString().data(),
        s_names, (unsigned)std::size(s_names), (int)PdfActionType::Unknown));

    switch (type)
    {
        case PdfActionType::GoTo:
            action = unique_ptr<PdfAction>(new PdfActionGoTo(obj));
            return true;
        case PdfActionType::GoToR:
            action = unique_ptr<PdfAction>(new PdfActionGoToR(obj));
            return true;
        case PdfActionType::GoToE:
            action = unique_ptr<PdfAction>(new PdfActionGoToE(obj));
            return true;
        case PdfActionType::Launch:
            action = unique_ptr<PdfAction>(new PdfActionLaunch(obj));
            return true;
        case PdfActionType::Thread:
            action = unique_ptr<PdfAction>(new PdfActionThread(obj));
            return true;
        case PdfActionType::URI:
            action = unique_ptr<PdfAction>(new PdfActionURI(obj));
            return true;
        case PdfActionType::Sound:
            action = unique_ptr<PdfAction>(new PdfActionSound(obj));
            return true;
        case PdfActionType::Movie:
            action = unique_ptr<PdfAction>(new PdfActionMovie(obj));
            return true;
        case PdfActionType::Hide:
            action = unique_ptr<PdfAction>(new PdfActionHide(obj));
            return true;
        case PdfActionType::Named:
            action = unique_ptr<PdfAction>(new PdfActionNamed(obj));
            return true;
        case PdfActionType::SubmitForm:
            action = unique_ptr<PdfAction>(new PdfActionSubmitForm(obj));
            return true;
        case PdfActionType::ResetForm:
            action = unique_ptr<PdfAction>(new PdfActionResetForm(obj));
            return true;
        case PdfActionType::ImportData:
            action = unique_ptr<PdfAction>(new PdfActionImportData(obj));
            return true;
        case PdfActionType::JavaScript:
            action = unique_ptr<PdfAction>(new PdfActionJavaScript(obj));
            return true;
        case PdfActionType::SetOCGState:
            action = unique_ptr<PdfAction>(new PdfActionSetOCGState(obj));
            return true;
        case PdfActionType::Rendition:
            action = unique_ptr<PdfAction>(new PdfActionRendition(obj));
            return true;
        case PdfActionType::Trans:
            action = unique_ptr<PdfAction>(new PdfActionTrans(obj));
            return true;
        case PdfActionType::GoTo3DView:
            action = unique_ptr<PdfAction>(new PdfActionGoTo3DView(obj));
            return true;
        case PdfActionType::RichMediaExecute:
            action = unique_ptr<PdfAction>(new PdfActionRichMediaExecute(obj));
            return true;
        case PdfActionType::Unknown:
        default:
            action.reset();
            return false;
    }
}

unique_ptr<PdfAction> PdfAction::Create(PdfDocument& doc, PdfActionType type)
{
    switch (type)
    {
        case PdfActionType::GoTo:
            return unique_ptr<PdfAction>(new PdfActionGoTo(doc));
        case PdfActionType::GoToR:
            return unique_ptr<PdfAction>(new PdfActionGoToR(doc));
        case PdfActionType::GoToE:
            return unique_ptr<PdfAction>(new PdfActionGoToE(doc));
        case PdfActionType::Launch:
            return unique_ptr<PdfAction>(new PdfActionLaunch(doc));
        case PdfActionType::Thread:
            return unique_ptr<PdfAction>(new PdfActionThread(doc));
        case PdfActionType::URI:
            return unique_ptr<PdfAction>(new PdfActionURI(doc));
        case PdfActionType::Sound:
            return unique_ptr<PdfAction>(new PdfActionSound(doc));
        case PdfActionType::Movie:
            return unique_ptr<PdfAction>(new PdfActionMovie(doc));
        case PdfActionType::Hide:
            return unique_ptr<PdfAction>(new PdfActionHide(doc));
        case PdfActionType::Named:
            return unique_ptr<PdfAction>(new PdfActionNamed(doc));
        case PdfActionType::SubmitForm:
            return unique_ptr<PdfAction>(new PdfActionSubmitForm(doc));
        case PdfActionType::ResetForm:
            return unique_ptr<PdfAction>(new PdfActionResetForm(doc));
        case PdfActionType::ImportData:
            return unique_ptr<PdfAction>(new PdfActionImportData(doc));
        case PdfActionType::JavaScript:
            return unique_ptr<PdfAction>(new PdfActionJavaScript(doc));
        case PdfActionType::SetOCGState:
            return unique_ptr<PdfAction>(new PdfActionSetOCGState(doc));
        case PdfActionType::Rendition:
            return unique_ptr<PdfAction>(new PdfActionRendition(doc));
        case PdfActionType::Trans:
            return unique_ptr<PdfAction>(new PdfActionTrans(doc));
        case PdfActionType::GoTo3DView:
            return unique_ptr<PdfAction>(new PdfActionGoTo3DView(doc));
        case PdfActionType::RichMediaExecute:
            return unique_ptr<PdfAction>(new PdfActionRichMediaExecute(doc));
        case PdfActionType::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported action");
    }
}

unique_ptr<PdfAction> PdfAction::Create(const PdfAction& action)
{
    switch (action.GetType())
    {
        case PdfActionType::GoTo:
            return unique_ptr<PdfAction>(new PdfActionGoTo(static_cast<const PdfActionGoTo&>(action)));
        case PdfActionType::GoToR:
            return unique_ptr<PdfAction>(new PdfActionGoToR(static_cast<const PdfActionGoToR&>(action)));
        case PdfActionType::GoToE:
            return unique_ptr<PdfAction>(new PdfActionGoToE(static_cast<const PdfActionGoToE&>(action)));
        case PdfActionType::Launch:
            return unique_ptr<PdfAction>(new PdfActionLaunch(static_cast<const PdfActionLaunch&>(action)));
        case PdfActionType::Thread:
            return unique_ptr<PdfAction>(new PdfActionThread(static_cast<const PdfActionThread&>(action)));
        case PdfActionType::URI:
            return unique_ptr<PdfAction>(new PdfActionURI(static_cast<const PdfActionURI&>(action)));
        case PdfActionType::Sound:
            return unique_ptr<PdfAction>(new PdfActionSound(static_cast<const PdfActionSound&>(action)));
        case PdfActionType::Movie:
            return unique_ptr<PdfAction>(new PdfActionMovie(static_cast<const PdfActionMovie&>(action)));
        case PdfActionType::Hide:
            return unique_ptr<PdfAction>(new PdfActionHide(static_cast<const PdfActionHide&>(action)));
        case PdfActionType::Named:
            return unique_ptr<PdfAction>(new PdfActionNamed(static_cast<const PdfActionNamed&>(action)));
        case PdfActionType::SubmitForm:
            return unique_ptr<PdfAction>(new PdfActionSubmitForm(static_cast<const PdfActionSubmitForm&>(action)));
        case PdfActionType::ResetForm:
            return unique_ptr<PdfAction>(new PdfActionResetForm(static_cast<const PdfActionResetForm&>(action)));
        case PdfActionType::ImportData:
            return unique_ptr<PdfAction>(new PdfActionImportData(static_cast<const PdfActionImportData&>(action)));
        case PdfActionType::JavaScript:
            return unique_ptr<PdfAction>(new PdfActionJavaScript(static_cast<const PdfActionJavaScript&>(action)));
        case PdfActionType::SetOCGState:
            return unique_ptr<PdfAction>(new PdfActionSetOCGState(static_cast<const PdfActionSetOCGState&>(action)));
        case PdfActionType::Rendition:
            return unique_ptr<PdfAction>(new PdfActionRendition(static_cast<const PdfActionRendition&>(action)));
        case PdfActionType::Trans:
            return unique_ptr<PdfAction>(new PdfActionTrans(static_cast<const PdfActionTrans&>(action)));
        case PdfActionType::GoTo3DView:
            return unique_ptr<PdfAction>(new PdfActionGoTo3DView(static_cast<const PdfActionGoTo3DView&>(action)));
        case PdfActionType::RichMediaExecute:
            return unique_ptr<PdfAction>(new PdfActionRichMediaExecute(static_cast<const PdfActionRichMediaExecute&>(action)));
        case PdfActionType::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported action");
    }
}

void PdfAction::AddToDictionary(PdfDictionary& dictionary) const
{
    dictionary.AddKey("A"_n, this->GetObject());
}

PdfActionGoTo::PdfActionGoTo(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::GoTo)
{
}

PdfActionGoTo::PdfActionGoTo(PdfObject& obj)
    : PdfAction(obj, PdfActionType::GoTo)
{
}

PdfActionGoToR::PdfActionGoToR(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::GoToR)
{
}

PdfActionGoToR::PdfActionGoToR(PdfObject& obj)
    : PdfAction(obj, PdfActionType::GoToR)
{
}

PdfActionGoToE::PdfActionGoToE(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::GoToE)
{
}

PdfActionGoToE::PdfActionGoToE(PdfObject& obj)
    : PdfAction(obj, PdfActionType::GoToE)
{
}

PdfActionLaunch::PdfActionLaunch(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Launch)
{
}

PdfActionLaunch::PdfActionLaunch(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Launch)
{
}

PdfActionThread::PdfActionThread(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Thread)
{
}

PdfActionThread::PdfActionThread(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Thread)
{
}

PdfActionURI::PdfActionURI(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::URI)
{
}

PdfActionURI::PdfActionURI(PdfObject& obj)
    : PdfAction(obj, PdfActionType::URI)
{
}

void PdfActionURI::SetURI(nullable<const PdfString&> uri)
{
    if (uri == nullptr)
        GetDictionary().RemoveKey("URI");
    else
        GetDictionary().AddKey("URI"_n, *uri);
}

nullable<const PdfString&> PdfActionURI::GetURI() const
{
    auto uriObj = GetDictionary().FindKey("URI");
    if (uriObj == nullptr)
        return { };
    else
        return uriObj->GetString();
}

PdfActionSound::PdfActionSound(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Sound)
{
}

PdfActionSound::PdfActionSound(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Sound)
{
}

PdfActionMovie::PdfActionMovie(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Movie)
{
}

PdfActionMovie::PdfActionMovie(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Movie)
{
}

PdfActionHide::PdfActionHide(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Hide)
{
}

PdfActionHide::PdfActionHide(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Hide)
{
}

PdfActionNamed::PdfActionNamed(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Named)
{
}

PdfActionNamed::PdfActionNamed(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Named)
{
}

PdfActionSubmitForm::PdfActionSubmitForm(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::SubmitForm)
{
}

PdfActionSubmitForm::PdfActionSubmitForm(PdfObject& obj)
    : PdfAction(obj, PdfActionType::SubmitForm)
{
}

PdfActionResetForm::PdfActionResetForm(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::ResetForm)
{
}

PdfActionResetForm::PdfActionResetForm(PdfObject& obj)
    : PdfAction(obj, PdfActionType::ResetForm)
{
}

PdfActionImportData::PdfActionImportData(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::ImportData)
{
}

PdfActionImportData::PdfActionImportData(PdfObject& obj)
    : PdfAction(obj, PdfActionType::ImportData)
{
}

PdfActionJavaScript::PdfActionJavaScript(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::JavaScript)
{
}

PdfActionJavaScript::PdfActionJavaScript(PdfObject& obj)
    : PdfAction(obj, PdfActionType::JavaScript)
{
}

void PdfActionJavaScript::SetScript(nullable<const PdfString&> script)
{
    if (script == nullptr)
        GetDictionary().RemoveKey("JS");
    else
        GetDictionary().AddKey("JS"_n, *script);
}

nullable<const PdfString&> PdfActionJavaScript::GetScript() const
{
    auto uriObj = GetDictionary().FindKey("JS");
    if (uriObj == nullptr)
        return { };
    else
        return uriObj->GetString();
}

PdfActionSetOCGState::PdfActionSetOCGState(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::SetOCGState)
{
}

PdfActionSetOCGState::PdfActionSetOCGState(PdfObject& obj)
    : PdfAction(obj, PdfActionType::SetOCGState)
{
}

PdfActionRendition::PdfActionRendition(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Rendition)
{
}

PdfActionRendition::PdfActionRendition(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Rendition)
{
}

PdfActionTrans::PdfActionTrans(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::Trans)
{
}

PdfActionTrans::PdfActionTrans(PdfObject& obj)
    : PdfAction(obj, PdfActionType::Trans)
{
}

PdfActionGoTo3DView::PdfActionGoTo3DView(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::GoTo3DView)
{
}

PdfActionGoTo3DView::PdfActionGoTo3DView(PdfObject& obj)
    : PdfAction(obj, PdfActionType::GoTo3DView)
{
}

PdfActionRichMediaExecute::PdfActionRichMediaExecute(PdfDocument& doc)
    : PdfAction(doc, PdfActionType::RichMediaExecute)
{
}

PdfActionRichMediaExecute::PdfActionRichMediaExecute(PdfObject& obj)
    : PdfAction(obj, PdfActionType::RichMediaExecute)
{
}
