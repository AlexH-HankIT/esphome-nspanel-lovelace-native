#include "todo_card.h"
#include "config.h"

namespace esphome {
namespace nspanel_lovelace {

std::string &TodoCard::render(std::string &buffer) {
  model_.set_page_size(Configuration::get_model() == nspanel_model_t::us_p ? 3 : 2);
  buffer.assign("entityUpd~").append(task_display_text(get_title(), 24));
  if (model_.page_count() > 1)
    buffer.append(" ").append(std::to_string(model_.page() + 1)).append("/")
        .append(std::to_string(model_.page_count()));
  if (model_.unconfirmed()) buffer.append(" - ").append(task_display_text(unconfirmed_, 16));
  buffer.append("~");
  if (model_.page() > 0)
    buffer.append("button~").append(nav_prefix()).append("prev~<~65535~~~");
  else if (nav_left)
    buffer.append(nav_left->render()).append("~");
  else
    buffer.append("delete~~~~~~");
  if (model_.page() + 1 < model_.page_count())
    buffer.append("button~").append(nav_prefix()).append("next~>~65535~~");
  else if (nav_right)
    buffer.append(nav_right->render());
  else
    buffer.append("delete~~~~~");

  const std::string *message = nullptr;
  if (!model_.error().empty()) message = &error_;
  else if (!model_.loaded()) message = &loading_;
  else if (model_.stale()) message = &stale_;
  else if (model_.items().empty()) message = &empty_;

  // Errors retain the last snapshot internally but disable its controls. A
  // fresh snapshot restores it. A confirmation timeout shows a dismissible
  // notice and the still-open tasks on the following refresh.
  if (message) {
    buffer.append("~text~task.info~~65535~").append(task_display_text(*message)).append("~");
    for (size_t i = 1; i < model_.page_size() * 2; ++i) buffer.append("~delete~~~~~");
    return buffer;
  }
  size_t begin = model_.page() * model_.page_size();
  for (size_t i = 0; i < model_.page_size(); ++i) {
    if (begin + i >= model_.items().size()) {
      buffer.append("~delete~~~~~~delete~~~~~");
      continue;
    }
    const auto &item = model_.items()[begin + i];
    buffer.append(item.pending ? "~text~task.info~" : "~button~" + task_prefix() + std::to_string(item.token) + "~")
        .append(CHAR8_CAST(icon_t::check_circle)).append("~")
        .append(item.overdue ? "63488" : "65535").append("~")
        .append(task_display_text(item.summary, 32)).append("~")
        .append(item.pending ? "..." : task_display_text(done_, 12));
    buffer.append("~text~task.info~~65535~")
        .append(task_display_text(item.due_text, 24)).append("~")
        .append(task_display_text(item.last_done_text, 24));
  }
  return buffer;
}

}  // namespace nspanel_lovelace
}  // namespace esphome
