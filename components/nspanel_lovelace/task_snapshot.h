#pragma once

#include "task_model.h"
#include <ArduinoJson.h>

namespace esphome {
namespace nspanel_lovelace {

// The caller chooses the allocator (PSRAM on the panel, ordinary RAM in host
// tests). Validate the complete snapshot before replacing live identities.
inline bool decode_task_snapshot(ArduinoJson::JsonDocument &doc,
                                 const std::string &payload,
                                 const std::vector<std::string> &allowed,
                                 TaskModel &model, uint32_t now) {
  if (payload.size() > TaskModel::MAX_PAYLOAD) return model.fail("Task payload exceeds 32 KiB");
  auto error = ArduinoJson::deserializeJson(doc, payload, ArduinoJson::DeserializationOption::NestingLimit(4));
  if (error || !doc.is<ArduinoJson::JsonObject>() || !doc["version"].is<int>() || doc["version"].as<int>() != 1)
    return model.fail("Invalid task snapshot or version");
  if (doc["error"].is<const char *>()) return model.fail(doc["error"].as<std::string>());
  if (!doc["items"].is<ArduinoJson::JsonArray>() || doc["items"].size() > TaskModel::MAX_ITEMS)
    return model.fail("Invalid task list or too many tasks");
  std::vector<TaskItem> items;
  for (ArduinoJson::JsonObjectConst value : doc["items"].as<ArduinoJson::JsonArrayConst>()) {
    if (!value["entity_id"].is<const char *>() || !value["uid"].is<const char *>() ||
        !value["summary"].is<const char *>() || !value["due_text"].is<const char *>() ||
        !value["last_done_text"].is<const char *>() || !value["overdue"].is<bool>())
      return model.fail("Task fields have invalid types");
    TaskItem item;
    item.entity_id = value["entity_id"].as<std::string>();
    item.uid = value["uid"].as<std::string>();
    item.summary = value["summary"].as<std::string>();
    item.due_text = value["due_text"].as<std::string>();
    item.last_done_text = value["last_done_text"].as<std::string>();
    item.overdue = value["overdue"].as<bool>();
    items.push_back(std::move(item));
  }
  return model.update(std::move(items), allowed, now);
}

}  // namespace nspanel_lovelace
}  // namespace esphome
