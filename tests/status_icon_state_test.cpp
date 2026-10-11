#include "status_icon_state.h"
#include <cassert>
#include <iostream>

using namespace esphome::nspanel_lovelace;

int main() {
  const std::string icon = u8"\uE755";
  for (const auto &count : {"0", "1", "9", "10", "32", "99"})
    assert(render_status_count(icon, 65535, count, false, true) == icon + " " + count + "~65535");
  assert(render_status_count(icon, 65535, "0", true, true) == "~65535");
  assert(render_status_count(icon, 65535, "000", true, true) == "~65535");
  assert(render_status_count(icon, 1234, "00032", true, true) == icon + " 32~1234");
  for (const auto &count : {"100", "99999999999999999999999999999999"})
    assert(render_status_count(icon, 65535, count, true, true) == icon + " 99+~65535");
  for (const auto &state : {"", "unknown", "unavailable", "-1", "1.5", "NaN", "1~0", "+2", " 2", "2\n"})
    assert(render_status_count(icon, 65535, state, true, true) == icon + " ?~38066");
  assert(render_status_count(icon, 65535, "0", true, false) == icon + " ?~38066");
  assert(render_status_count(icon, 65535, "1", false, false) == icon + " ?~38066");
  assert(render_status_count(icon, 65535, "0", true, true) == "~65535"); // fresh recovery
  assert(render_status_icons("~", icon + " 32~65535", false, false) ==
      "statusUpdate~~~" + icon + " 32~65535~~");
  assert(render_status_icons(icon + "~1234", "~", true, false) ==
      "statusUpdate~" + icon + "~1234~~~1~");
  assert(render_status_icons("left~42", "right~84", false, true) ==
      "statusUpdate~left~42~right~84~~1");
  assert(render_status_icons("~", "~65535", false, false) == "statusUpdate~~~~65535~~");
  std::cout << "Status icon count and protocol checks passed\n";
}
