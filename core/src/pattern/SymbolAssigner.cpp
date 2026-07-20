#include "ss/core/pattern/SymbolAssigner.h"

#include <QSet>

namespace ss::core::pattern {

namespace {

// Ordered for visual distinctiveness at small sizes: digits and case-mixed
// letters first, then punctuation that's unlikely to be confused with grid
// lines or each other. All members are single Latin-1 characters available
// in Helvetica's standard encoding.
const QString& charset() {
    static const QString kCharset =
        QStringLiteral("0123456789"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "+-=*/\\?!@#$%^&~<>()[]{}|;:,.");
    return kCharset;
}

} // namespace

QMap<QString, QString> SymbolAssigner::assign(const QVector<QString>& codes,
                                               const QMap<QString, QString>& existing) {
    QMap<QString, QString> result;
    QSet<QString> usedSymbols;

    for (const QString& code : codes) {
        auto it = existing.find(code);
        if (it != existing.end()) {
            result.insert(code, it.value());
            usedSymbols.insert(it.value());
        }
    }

    const QString& chars = charset();
    int nextSingle = 0;
    int fallbackPairFirst = 0;
    int fallbackPairSecond = 0;

    for (const QString& code : codes) {
        if (result.contains(code)) continue;

        QString symbol;
        while (nextSingle < chars.size()) {
            const QString candidate(chars[nextSingle]);
            ++nextSingle;
            if (!usedSymbols.contains(candidate)) {
                symbol = candidate;
                break;
            }
        }

        if (symbol.isEmpty()) {
            // Charset exhausted: fall back to two-character combinations.
            while (fallbackPairFirst < chars.size()) {
                const QString candidate = QString(chars[fallbackPairFirst]) + chars[fallbackPairSecond];
                ++fallbackPairSecond;
                if (fallbackPairSecond >= chars.size()) {
                    fallbackPairSecond = 0;
                    ++fallbackPairFirst;
                }
                if (!usedSymbols.contains(candidate)) {
                    symbol = candidate;
                    break;
                }
            }
        }

        result.insert(code, symbol);
        usedSymbols.insert(symbol);
    }

    return result;
}

} // namespace ss::core::pattern
