#pragma once

#include "cards.h"
#include "task_model.h"

namespace esphome {
namespace nspanel_lovelace {

// Uses the existing EntitiesCard visitor and TFT page; tasks are not HA entity
// subscriptions and must not enter the cached StatefulPageItem collections.
class TodoCard : public EntitiesCard {
 public:
  TodoCard(const std::string &uuid, const std::string &title, uint16_t timeout)
      : EntitiesCard(uuid, title, timeout) {}
  void set_lists(const std::vector<std::string> &lists) { lists_ = lists; }
  void set_completion_script(const std::string &script) { completion_script_ = script; }
  void set_refresh_script(const std::string &script) { refresh_script_ = script; }
  void set_labels(const std::string &done, const std::string &loading,
                  const std::string &empty, const std::string &stale,
                  const std::string &error, const std::string &unconfirmed) {
    done_ = done; loading_ = loading; empty_ = empty;
    stale_ = stale; error_ = error; unconfirmed_ = unconfirmed;
  }
  TaskModel &model() { return model_; }
  const std::vector<std::string> &lists() const { return lists_; }
  const std::string &completion_script() const { return completion_script_; }
  const std::string &refresh_script() const { return refresh_script_; }
  std::string task_prefix() const { return "task." + get_uuid() + "."; }
  std::string nav_prefix() const { return "tasknav." + get_uuid() + "."; }
  std::string &render(std::string &buffer) override;

 private:
  TaskModel model_;
  std::vector<std::string> lists_;
  std::string completion_script_, refresh_script_;
  std::string done_{"Done"}, loading_{"Loading"}, empty_{"No open tasks"},
      stale_{"Data stale"}, error_{"Sync error"}, unconfirmed_{"Not confirmed"};
};

}  // namespace nspanel_lovelace
}  // namespace esphome
