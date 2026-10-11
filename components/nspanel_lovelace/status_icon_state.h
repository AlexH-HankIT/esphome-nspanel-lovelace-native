#pragma once

#include <cstdint>
#include <string>

namespace esphome {
namespace nspanel_lovelace {

// Counts are deliberately narrow: no units, signs, decimals or protocol text.
// Saturate while parsing so arbitrarily large integer states cannot overflow.
inline std::string render_status_count(const std::string &icon, uint16_t color,
                                       const std::string &state, bool hide_zero,
                                       bool available) {
  unsigned count = 0;
  bool valid = available && !state.empty();
  for (unsigned char c : state) {
    if (c < '0' || c > '9') { valid = false; break; }
    if (count < 100) count = count * 10 + (c - '0');
  }
  if (!valid) return icon + " ?~38066";
  if (hide_zero && count == 0) return "~" + std::to_string(color);
  return icon + " " + (count > 99 ? "99+" : std::to_string(count)) + "~" + std::to_string(color);
}

// Preserve the display protocol's empty fields when one side is absent.
inline std::string render_status_icons(const std::string &left, const std::string &right,
                                       bool left_alt, bool right_alt) {
  return "statusUpdate~" + left + "~" + right + "~" +
      (left_alt ? "1" : "") + "~" + (right_alt ? "1" : "");
}

}  // namespace nspanel_lovelace
}  // namespace esphome
