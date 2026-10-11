#include "pages.h"
#include "status_icon_state.h"

#include "config.h"
#include "types.h"

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Screensaver ===============
 */

void Screensaver::accept(PageVisitor& visitor) { visitor.visit(*this); }

void Screensaver::set_icon_left(std::shared_ptr<StatusIconItem> left_icon) {
  this->left_icon = std::move(left_icon);
}
void Screensaver::set_icon_right(std::shared_ptr<StatusIconItem> right_icon) {
  this->right_icon = std::move(right_icon);
}

// output: weatherUpd~(5x)[type~internalName~icon~iconColor~displayName~value]
std::string &Screensaver::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction());

  for (auto &item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }
  
  return buffer;
}

// output: statusUpdate~icon1~icon1Color~icon2~icon2Color~icon1AltFont~icon2AltFont
std::string &Screensaver::render_status_update(std::string &buffer) {
  if (!this->left_icon && !this->right_icon) {
    buffer.clear();
    return buffer;
  }
  return buffer = render_status_icons(
      this->left_icon ? this->left_icon->render() : "~",
      this->right_icon ? this->right_icon->render() : "~",
      this->left_icon && this->left_icon->get_alt_font(),
      this->right_icon && this->right_icon->get_alt_font());
}

} // namespace nspanel_lovelace
} // namespace esphome