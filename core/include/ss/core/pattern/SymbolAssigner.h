#pragma once

#include <QMap>
#include <QString>
#include <QVector>

namespace ss::core::pattern {

// Assigns each DMC color in a pattern a single-character symbol drawn from
// a charset restricted to what base-14 Helvetica can render (digits,
// letters, and standard punctuation) — no dingbat/symbol-font glyphs, so the
// on-screen grid and the PDF export always agree exactly.
class SymbolAssigner {
public:
    // Assigns a symbol to every code in `codes`. Codes already present in
    // `existing` keep their symbol (stable across edits); new codes get the
    // next unused charset entry. If more codes are given than the charset
    // has entries, remaining codes get two-character symbols reusing the
    // charset.
    static QMap<QString, QString> assign(const QVector<QString>& codes,
                                          const QMap<QString, QString>& existing = {});
};

} // namespace ss::core::pattern
