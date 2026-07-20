#include "ss/core/dmc/DmcTable.h"

namespace ss::core::dmc {

DmcTable::DmcTable() {
    m_colors.assign(kDmcTableData, kDmcTableData + kDmcTableSize);
}

std::optional<DmcColor> DmcTable::findByCode(const std::string& code) const {
    for (const auto& c : m_colors) {
        if (code == c.code) {
            return c;
        }
    }
    return std::nullopt;
}

} // namespace ss::core::dmc
