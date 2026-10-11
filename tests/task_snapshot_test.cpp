#include "task_snapshot.h"
#include <cassert>
#include <iostream>

using namespace esphome::nspanel_lovelace;

int main() {
  ArduinoJson::JsonDocument doc;
  TaskModel model;
  const std::vector<std::string> allowed{"todo.maintenance"};
  const std::string item = R"({"entity_id":"todo.maintenance","uid":"one","summary":"Überfällige Aufgabe","due_text":"Fällig heute","last_done_text":"Zuletzt 09.10.2026","overdue":false})";
  auto decode = [&](const std::string &value) { return decode_task_snapshot(doc, value, allowed, model, 100); };
  assert(decode("{\"version\":1,\"items\":[" + item + "]}"));
  assert(model.items()[0].summary == "Überfällige Aufgabe");
  const auto token = model.items()[0].token;
  for (const auto &invalid : std::vector<std::string>{
       "not json", "null", "[]", "{}", "{\"version\":2,\"items\":[]}",
       "{\"version\":1,\"items\":[null]}",
       "{\"version\":1,\"items\":[{}]}",
       "{\"version\":1,\"items\":[" + item + "," + item + "]}",
       "{\"version\":1,\"error\":\"HA unavailable\"}",
       std::string(TaskModel::MAX_PAYLOAD + 1, ' ')}) {
    assert(!decode(invalid));
    assert(model.items().size() == 1 && model.items()[0].token == token);
  }
  auto wrong_type = item;
  wrong_type.replace(wrong_type.find("false"), 5, "\"false\"");
  assert(!decode("{\"version\":1,\"items\":[" + wrong_type + "]}"));
  auto unauthorized = item;
  unauthorized.replace(unauthorized.find("todo.maintenance"), 16, "todo.other");
  assert(!decode("{\"version\":1,\"items\":[" + unauthorized + "]}"));
  assert(decode("{\"version\":1,\"items\":[]}"));
  assert(model.items().empty() && model.error().empty());
  std::cout << "Task snapshot checks passed\n";
}
