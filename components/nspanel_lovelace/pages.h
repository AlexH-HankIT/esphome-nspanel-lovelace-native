#pragma once

#include "page_base.h"
#include "page_items.h"
#include "page_visitor.h"
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Screensaver ===============
 */

class Screensaver : public Page {
public:
  Screensaver(const std::string &uuid) : Page(page_type::screensaver, uuid) {}
  virtual ~Screensaver() {}

  void accept(PageVisitor& visitor) override;

  void set_icon_left(std::shared_ptr<StatusIconItem> left_icon);
  void set_icon_right(std::shared_ptr<StatusIconItem> right_icon);
  bool has_status_count() const {
    return (this->left_icon && this->left_icon->get_show_state()) ||
           (this->right_icon && this->right_icon->get_show_state());
  }
  bool set_status_connection(bool connected) {
    bool changed = this->left_icon && this->left_icon->set_connected(connected);
    if (this->right_icon) changed = this->right_icon->set_connected(connected) || changed;
    return changed;
  }
  void mark_status_state_received(const std::string &entity_id) {
    for (auto &icon : {this->left_icon, this->right_icon})
      if (icon && icon->get_entity_id() == entity_id) icon->mark_state_received();
  }
  bool should_render_status_update(const std::string &entity_id = "") {
    if (this->left_icon && (entity_id.empty() ||
        this->left_icon->get_entity_id() == entity_id)) {
      return true;
    }
    if (this->right_icon && (entity_id.empty() ||
        this->right_icon->get_entity_id() == entity_id)) {
      return true;
    }
    return false;
  }
  
  const char *get_render_instruction() const override { return "weatherUpdate"; };
  std::string &render(std::string &buffer) override;

  virtual std::string &render_status_update(std::string &buffer);

protected:
  std::shared_ptr<StatusIconItem> left_icon;
  std::shared_ptr<StatusIconItem> right_icon;
};

} // namespace nspanel_lovelace
} // namespace esphome