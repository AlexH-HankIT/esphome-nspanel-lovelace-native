#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

// Independent of ESPHome so identity, pagination and confirmation can be tested
// without a panel. Occurrence identities are (list entity, UID), never row slots.
struct TaskItem {
  std::string entity_id, uid, summary, due_text, last_done_text;
  bool overdue{false};
  uint64_t token{0};
  bool pending{false};
  uint32_t requested_at{0};
};

inline std::string task_display_text(const std::string &text, size_t max_chars = 48) {
  std::string result;
  size_t chars = 0;
  for (unsigned char c : text) {
    if ((c & 0xC0) != 0x80 && ++chars > max_chars) break;
    result += (c < 32 || c == 127 || c == '~') ? ' ' : static_cast<char>(c);
  }
  return result;
}

class TaskModel {
 public:
  static constexpr size_t MAX_ITEMS = 32;
  static constexpr size_t MAX_PAYLOAD = 32768;
  static constexpr uint32_t STALE_MS = 120000;
  static constexpr uint32_t CONFIRM_MS = 10000;

  bool update(std::vector<TaskItem> incoming, const std::vector<std::string> &allowed,
              uint32_t now) {
    if (incoming.size() > MAX_ITEMS) return fail("Too many tasks");
    for (size_t i = 0; i < incoming.size(); ++i) {
      const auto &item = incoming[i];
      if (item.uid.empty() || item.uid.size() > 255 || item.entity_id.size() > 255 ||
          item.summary.empty() || item.summary.size() > 2048 ||
          item.due_text.size() > 512 || item.last_done_text.size() > 512 ||
          std::find(allowed.begin(), allowed.end(), item.entity_id) == allowed.end())
        return fail("Invalid task identity or text");
      for (size_t j = 0; j < i; ++j)
        if (incoming[j].entity_id == item.entity_id && incoming[j].uid == item.uid)
          return fail("Duplicate task identity");
    }
    for (auto &item : incoming) {
      auto old = std::find_if(items_.begin(), items_.end(), [&item](const TaskItem &v) {
        return item.entity_id == v.entity_id && item.uid == v.uid;
      });
      if (old != items_.end()) {
        item.token = old->token;
        item.pending = old->pending;
        item.requested_at = old->requested_at;
      } else {
        item.token = ++next_token_;
      }
    }
    items_ = std::move(incoming);
    loaded_ = true;
    stale_ = false;
    last_update_ = now;
    error_.clear();
    unconfirmed_ = false;
    page_ = std::min(page_, page_count() - 1);
    return true;
  }

  bool fail(const std::string &message) {
    error_ = message.empty() ? "Synchronization failed" : message;
    return false;
  }
  bool tick(uint32_t now, bool connected) {
    bool changed = false;
    const bool stale = !connected || (loaded_ && uint32_t(now - last_update_) >= STALE_MS);
    if (stale != stale_) { stale_ = stale; changed = true; }
    for (auto &item : items_)
      if (item.pending && uint32_t(now - item.requested_at) >= CONFIRM_MS) {
        item.pending = false;
        unconfirmed_ = true;
        changed = true;
      }
    return changed;
  }
  const TaskItem *request(const std::string &token, uint32_t now) {
    if (!loaded_ || stale_ || !error_.empty()) return nullptr;
    for (auto &item : items_)
      if (std::to_string(item.token) == token && !item.pending) {
        unconfirmed_ = false;
        item.pending = true;
        item.requested_at = now;
        return &item;
      }
    return nullptr;
  }
  void set_page_size(size_t size) {
    page_size_ = std::max(size_t(1), size);
    page_ = std::min(page_, page_count() - 1);
  }
  bool previous() { if (page_ == 0) return false; --page_; return true; }
  bool next() { if (page_ + 1 >= page_count()) return false; ++page_; return true; }
  size_t page_count() const { return std::max(size_t(1), (items_.size() + page_size_ - 1) / page_size_); }
  size_t page() const { return page_; }
  size_t page_size() const { return page_size_; }
  const std::vector<TaskItem> &items() const { return items_; }
  bool loaded() const { return loaded_; }
  bool stale() const { return stale_; }
  bool unconfirmed() const { return unconfirmed_; }
  const std::string &error() const { return error_; }

 private:
  std::vector<TaskItem> items_;
  uint64_t next_token_{0};
  size_t page_{0}, page_size_{2};
  uint32_t last_update_{0};
  bool loaded_{false}, stale_{false}, unconfirmed_{false};
  std::string error_;
};

}  // namespace nspanel_lovelace
}  // namespace esphome
