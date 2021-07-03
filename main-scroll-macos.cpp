#include <locale.h>
#include <ncurses.h>
#include <optional>
#include <panel.h>
#include <stack>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char** argv) {
  // enable support for unicode characters
  setlocale(LC_CTYPE, "");

  initscr(); // start curses mode
  cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  curs_set(0); // hide cursor

  struct entity_t {
    std::string name_;
  };

  std::vector<entity_t> entities;
  std::generate_n(std::back_inserter(entities), 20, [number = 1]() mutable {
    return entity_t{std::string("entity_") + std::to_string(number++)};
  });

  struct view_t {
    int offset = 0;
    int count = 30;
  };

  view_t view;
  for (bool running = true; running;) {
    clear();

    const int count = std::min((int)entities.size(), view.offset + view.count);
    for (int entity_index = view.offset;
         entity_index < count; ++entity_index) {
      mvprintw(
        entity_index - view.offset, 0, entities[entity_index].name_.c_str());
    }

    refresh();
    move(0, 0);

    enum class input_e { up, down };

    switch (int key = getch(); key) {
      case KEY_UP:
        view.offset = std::max(view.offset - 1, 0);
        break;
      case KEY_DOWN: {
        int temp = std::max((int)entities.size() - view.count, 0);
        view.offset = std::min(view.offset + 1, temp);
      } break;
      default:
        // noop
        break;
    }
  }

  endwin();

  return 0;
}
