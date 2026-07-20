#include "HpdfDocument.h"

namespace ss::core::pdf {

namespace {

extern "C" void hpdfErrorHandler(HPDF_STATUS errorNo, HPDF_STATUS detailNo, void* userData) {
    static_cast<HpdfDocument*>(userData)->recordError(errorNo, detailNo);
}

} // namespace

HpdfDocument::HpdfDocument() {
    m_doc = HPDF_New(hpdfErrorHandler, this);
    if (!m_doc) {
        m_hasError = true;
        m_errorMessage = QStringLiteral("Failed to initialize libharu document");
    }
}

HpdfDocument::~HpdfDocument() {
    if (m_doc) {
        HPDF_Free(m_doc);
    }
}

void HpdfDocument::recordError(HPDF_STATUS errorNo, HPDF_STATUS detailNo) {
    // Once broken, later libharu calls tend to cascade further errors; keep
    // only the first one, since that's the actual root cause.
    if (m_hasError) return;
    m_hasError = true;
    m_errorMessage = QStringLiteral("libharu error 0x%1 (detail 0x%2)")
                          .arg(static_cast<unsigned long>(errorNo), 0, 16)
                          .arg(static_cast<unsigned long>(detailNo), 0, 16);
}

bool HpdfDocument::saveToFile(const QString& path) {
    if (m_hasError) return false;
    const HPDF_STATUS status = HPDF_SaveToFile(m_doc, path.toLocal8Bit().constData());
    return status == HPDF_OK && !m_hasError;
}

} // namespace ss::core::pdf
