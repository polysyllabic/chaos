#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include <Controller.hpp>
#include <GameMenu.hpp>
#include <MenuInterface.hpp>
#include <MenuItem.hpp>
#include <Sequence.hpp>

using namespace Chaos;

class MockMenu : public MenuInterface {
public:
  std::unordered_map<std::string, std::shared_ptr<MenuItem>> items;
  std::unordered_map<std::string, int> step_counts;
  int offset_updates = 0;

  std::shared_ptr<MenuItem> getMenuItem(const std::string& name) override {
    auto it = items.find(name);
    if (it == items.end()) {
      return nullptr;
    }
    return it->second;
  }

  void correctOffset(std::shared_ptr<MenuItem> sender) override {
    (void) sender;
    ++offset_updates;
  }

  void addToSequence(Sequence& sequence, const std::string& name) override {
    (void) sequence;
    ++step_counts[name];
  }
};

static bool check(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

static bool testSelectUsesCorrectedOffset() {
  MockMenu mock;
  Controller controller;
  Sequence seq(controller);

  auto item = std::make_shared<MenuItem>(
      mock, "item", 1, 0, 0, false,
      true, false, false, false,
      nullptr, nullptr, nullptr, CounterAction::NONE);
  mock.items["item"] = item;

  item->adjustOffset(-1);
  item->selectItem(seq);

  return check(mock.step_counts["menu down"] == 0,
               "selectItem should use corrected offset (menu_down count)");
}

static bool testNavigateBackUsesCorrectedOffset() {
  MockMenu mock;
  Controller controller;
  Sequence seq(controller);

  auto item = std::make_shared<MenuItem>(
      mock, "item", 1, 0, 0, false,
      true, false, false, false,
      nullptr, nullptr, nullptr, CounterAction::NONE);
  mock.items["item"] = item;

  item->adjustOffset(-1);
  item->navigateBack(seq);

  bool ok = true;
  ok &= check(mock.step_counts["menu up"] == 0,
              "navigateBack should use corrected offset (menu_up count)");
  ok &= check(mock.step_counts["menu down"] == 0,
              "navigateBack should use corrected offset (menu_down count)");
  ok &= check(mock.step_counts["menu exit"] == 1,
              "navigateBack should always emit one menu_exit");
  return ok;
}

static bool testGuardedVisibilitySync() {
  bool ok = true;

  {
    GameMenu menu;
    menu.setHideGuarded(true);
    auto guard = std::make_shared<MenuItem>(
        menu, "guard", 0, 0, 0, false,
        true, false, false, false,
        nullptr, nullptr, nullptr, CounterAction::NONE);
    auto guarded = std::make_shared<MenuItem>(
        menu, "guarded", 1, 0, 0, false,
        true, false, false, false,
        nullptr, guard, nullptr, CounterAction::NONE);
    std::string gname = "guard";
    std::string iname = "guarded";
    menu.insertMenuItem(gname, guard);
    menu.insertMenuItem(iname, guarded);

    menu.syncGuardedVisibility();
    ok &= check(guarded->isHidden(),
                "guarded item should be hidden when hide_guarded_items=true and guard is off");
  }

  {
    GameMenu menu;
    menu.setHideGuarded(true);
    auto guard = std::make_shared<MenuItem>(
        menu, "guard", 0, 0, 1, false,
        true, false, false, false,
        nullptr, nullptr, nullptr, CounterAction::NONE);
    auto guarded = std::make_shared<MenuItem>(
        menu, "guarded", 1, 0, 0, false,
        true, false, false, false,
        nullptr, guard, nullptr, CounterAction::NONE);
    std::string gname = "guard";
    std::string iname = "guarded";
    menu.insertMenuItem(gname, guard);
    menu.insertMenuItem(iname, guarded);

    menu.syncGuardedVisibility();
    ok &= check(!guarded->isHidden(),
                "guarded item should be visible when guard is on");
  }

  return ok;
}

static bool testZeroResetCounterAction() {
  MockMenu mock;
  Controller controller;
  Sequence seq(controller);
  bool ok = true;

  auto item = std::make_shared<MenuItem>(
      mock, "counter_item", 0, 0, 2, false,
      true, false, false, false,
      nullptr, nullptr, nullptr, CounterAction::ZERO_RESET);
  mock.items["counter_item"] = item;

  item->setState(seq, 5, false);
  ok &= check(item->getState() == 5, "setState should update option state");

  item->setCounter(1);
  item->decrementCounter();
  ok &= check(item->getState() == item->getDefault(),
              "ZERO_RESET should reset state to default when counter reaches zero");

  return ok;
}

static bool testSelectCounterDirection() {
  MockMenu mock;
  Controller controller;
  Sequence seq(controller);
  bool ok = true;

  auto counter_item = std::make_shared<MenuItem>(
      mock, "counter", 0, 0, 0, false,
      true, false, false, false,
      nullptr, nullptr, nullptr, CounterAction::NONE);
  auto select_item = std::make_shared<MenuItem>(
      mock, "select_item", 0, 0, 0, false,
      false, true, false, false,
      nullptr, nullptr, counter_item, CounterAction::NONE);
  auto option_item = std::make_shared<MenuItem>(
      mock, "option_item", 0, 0, 0, false,
      true, false, false, false,
      nullptr, nullptr, counter_item, CounterAction::NONE);

  counter_item->setCounter(1);
  select_item->setState(seq, 0, false);
  ok &= check(counter_item->getCounter() == 2,
              "select begin should increment sibling counter");
  select_item->setState(seq, 0, true);
  ok &= check(counter_item->getCounter() == 1,
              "select restore should decrement sibling counter");

  option_item->setState(seq, 0, false);
  ok &= check(counter_item->getCounter() == 2,
              "option begin should increment sibling counter");
  option_item->setState(seq, 0, true);
  ok &= check(counter_item->getCounter() == 1,
              "option restore should decrement sibling counter");

  return ok;
}

int main() {
  bool ok = true;
  ok &= testSelectUsesCorrectedOffset();
  ok &= testNavigateBackUsesCorrectedOffset();
  ok &= testGuardedVisibilitySync();
  ok &= testZeroResetCounterAction();
  ok &= testSelectCounterDirection();

  if (!ok) {
    return 1;
  }
  std::cout << "PASS: menu sequence tests\n";
  return 0;
}
