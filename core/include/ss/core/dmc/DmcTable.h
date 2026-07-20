#pragma once

#include "ss/core/dmc/DmcColor.h"

#include <optional>
#include <string>
#include <vector>

namespace ss::core::dmc {

// Generated data, compiled in (see DmcTableData.cpp).
extern const DmcColor kDmcTableData[];
extern const int kDmcTableSize;

// Read-only view over the full DMC floss range.
class DmcTable {
public:
    DmcTable();

    const std::vector<DmcColor>& colors() const { return m_colors; }

    std::optional<DmcColor> findByCode(const std::string& code) const;

private:
    std::vector<DmcColor> m_colors;
};

} // namespace ss::core::dmc
