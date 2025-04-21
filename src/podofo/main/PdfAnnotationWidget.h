/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ANNOTATION_WIDGET_H
#define PDF_ANNOTATION_WIDGET_H

#include "PdfAnnotationActionBase.h"

namespace PoDoFo {

    class PdfField;

    class PODOFO_API PdfAnnotationWidget :
        public PdfAnnotationActionBase,
        public PdfAppearanceCharacteristicsProvider<PdfAnnotationWidget>
    {
        friend class PdfAnnotation;
        friend class PdfField;
        friend class PdfPage;
    private:
        PdfAnnotationWidget(PdfPage& page, const Rect& rect);
        PdfAnnotationWidget(PdfObject& obj);
    public:
        const PdfField& GetField() const;
        PdfField& GetField();
    private:
        void SetField(std::shared_ptr<PdfField>&& field);
        const std::shared_ptr<PdfField>& GetFieldPtr() { return m_Field; }
    private:
        void initField();
    private:
        std::shared_ptr<PdfField> m_Field;
    };
}

#endif // PDF_ANNOTATION_WIDGET_H
