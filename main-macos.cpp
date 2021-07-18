#include "hierarchy/entity.hpp"

#include <locale.h>
#include <ncurses.h>
#include <optional>
#include <panel.h>
#include <stack>
#include <string.h>
#include <utility>

int main(int argc, char** argv) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);

  // enable support for unicode characters
  setlocale(LC_CTYPE, "");

  initscr(); // start curses mode
  cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  curs_set(0); // hide cursor

  hy::collapser_t collapser;
  for (const auto& handle : root_handles) {
    collapser.collapse(handle, entities);
  }

  hy::view_t view(
    hy::flatten_entities(entities, collapser, root_handles), 0, 20);

  for (bool running = true; running;) {
    clear();

    const int count = std::min(
      (int)view.flattened_handles().size(), view.offset() + view.count());

    for (int handle_index = view.offset(); handle_index < count;
         ++handle_index) {
      const auto& flattened_handle = view.flattened_handles()[handle_index];
      if (handle_index == view.selected()) {
        attron(A_REVERSE);
      }
      if (collapser.collapsed(flattened_handle.entity_handle_)) {
        attron(A_BOLD);
      }
      entities.call(flattened_handle.entity_handle_, [&](const auto& entity) {
        mvprintw(
          handle_index - view.offset(), flattened_handle.indent_ * 2,
          entity.name_.c_str());
      });
      attroff(A_REVERSE);
      attroff(A_BOLD);
    }

    refresh();
    move(0, 0);

    switch (const int key = getch(); key) {
      case KEY_UP:
        view.move_up();
        break;
      case KEY_DOWN:
        view.move_down();
        break;
      case KEY_LEFT:
        view.collapse(entities, collapser);
        break;
      case KEY_RIGHT:
        view.expand(entities, collapser);
        break;
      case 'g':
        view.goto_handle(entities, collapser);
        break;
      case 'r':
        view.record_handle();
        break;
      case 'c':
        view.add_child(entities, collapser);
        break;
      case 's':
        view.add_sibling(entities, collapser, root_handles);
        break;
      default:
        // noop
        break;
    }
  }

  endwin();

  return 0;
}
