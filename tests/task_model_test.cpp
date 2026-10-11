#include "task_model.h"
#include <cassert>
#include <iostream>

using namespace esphome::nspanel_lovelace;

TaskItem task(const std::string &uid) {
  TaskItem result;
  result.entity_id = "todo.maintenance";
  result.uid = uid;
  result.summary = "Beispielaufgabe";
  return result;
}

int main() {
  const std::vector<std::string> lists{"todo.maintenance"};
  TaskModel m;
  assert(!m.loaded());
  assert(m.update({task("a"), task("b"), task("c")}, lists, 100));
  assert(m.page_count() == 2 && m.next() && !m.next());
  auto a = std::to_string(m.items()[0].token);
  auto b = std::to_string(m.items()[1].token);
  assert(m.request(a, 200));
  assert(!m.request(a, 201));
  // A moved row retains identity and pending state. Deleting another occurrence
  // must not redirect its old touch token to the replacement row.
  assert(m.update({task("b"), task("a")}, lists, 300));
  assert(m.page() == 0 && m.items()[1].pending);
  assert(m.update({task("a"), task("d")}, lists, 400));
  assert(!m.request(b, 401));
  assert(m.tick(10200, true));
  assert(m.unconfirmed() && !m.items()[0].pending);
  assert(m.request(a, 10201));
  // A deleted and subsequently recreated identity receives a new token.
  assert(m.update({}, lists, 10300));
  assert(m.update({task("a")}, lists, 10400));
  assert(!m.request(a, 10401));
  assert(!m.update({task("x"), task("x")}, lists, 10500));
  assert(m.items().size() == 1 && !m.error().empty());
  assert(!m.request(std::to_string(m.items()[0].token), 10501));
  assert(!m.update({task("z")}, {"todo.other"}, 10600));
  assert(!m.update(std::vector<TaskItem>(33, task("z")), lists, 10700));
  assert(m.update({task("a")}, lists, 10800));
  assert(m.tick(130800, true) && m.stale());
  assert(!m.request(std::to_string(m.items()[0].token), 130801));
  assert(m.update({task("a")}, lists, 131000));
  assert(m.tick(131001, false) && m.stale());
  // Unsigned subtraction must handle the embedded millisecond clock wrapping.
  assert(m.update({task("wrap")}, lists, UINT32_MAX - 100));
  assert(m.request(std::to_string(m.items()[0].token), UINT32_MAX - 50));
  assert(m.tick(10000, true) && !m.items()[0].pending);
  assert(!m.stale());
  assert(task_display_text("Ä~\n中文", 4) == "Ä  中");
  assert(task_display_text("Überfällig", 4) == "Über");
  std::cout << "Task model checks passed\n";
}
