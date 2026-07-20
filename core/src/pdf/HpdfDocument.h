#pragma once

#include <hpdf.h>

#include <QString>

namespace ss::core::pdf {

// RAII wrapper around HPDF_Doc. Deliberately does not use libharu's
// setjmp/longjmp error-recovery idiom (jumping out from underneath live
// C++ stack frames skips destructors); instead the error handler just
// records the failure and callers check hasError() at checkpoints, bailing
// out of the export before anything gets written to disk.
class HpdfDocument {
public:
    HpdfDocument();
    ~HpdfDocument();

    HpdfDocument(const HpdfDocument&) = delete;
    HpdfDocument& operator=(const HpdfDocument&) = delete;

    HPDF_Doc handle() const { return m_doc; }

    bool hasError() const { return m_hasError; }
    QString errorMessage() const { return m_errorMessage; }

    bool saveToFile(const QString& path);

    void recordError(HPDF_STATUS errorNo, HPDF_STATUS detailNo);

private:
    HPDF_Doc m_doc = nullptr;
    bool m_hasError = false;
    QString m_errorMessage;
};

} // namespace ss::core::pdf
