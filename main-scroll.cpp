#include "hierarchy/entity.hpp"

#include <algorithm>
#include <locale.h>
#include <optional>
#include <stack>
#include <string.h>
#include <utility>

#ifdef _WIN32
#include "third-party/pdcurses/curses.h"
#elif defined(__unix__) || defined(__APPLE__)
#include <ncurses.h>
#endif

int main(int argc, char** argv) {
  thh::handle_vector_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);
  // auto root_handles = demo::create_bench_entities(entities, 5, 10000);

  // enable support for unicode characters
  setlocale(LC_CTYPE, "");

  initscr(); // start curses mode
  cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  curs_set(0); // hide cursor

  hy::collapser_t collapser;
  // temp - keep root handles expanded
  // for (const auto& handle : root_handles) {
  //   collapser.collapse(handle, entities);
  // }

  hy::display_ops_t display_ops;
  display_ops.connection_ = "\xE2\x94\x82";
  display_ops.end_ = "\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80 ";
  display_ops.mid_ = "\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80 ";
  display_ops.indent_width_ = 4;

  display_ops.set_bold_fn_ = [](const bool bold) {
    if (bold) {
      attron(A_BOLD);
    } else {
      attroff(A_BOLD);
    }
  };
  display_ops.set_invert_fn_ = [](const bool invert) {
    if (invert) {
      attron(A_REVERSE);
    } else {
      attroff(A_REVERSE);
    }
  };
  display_ops.draw_fn_ = [](const std::string_view str) {
    printw("%.*s", int(str.length()), str.data());
  };
  display_ops.draw_at_fn_ =
    [](const int x, const int y, const std::string_view str) {
      mvprintw(y, x, "%.*s", int(str.length()), str.data());
    };

  hy::view_t view(
    hy::flatten_entities(entities, collapser, root_handles), 0, 10);

  for (bool running = true; running;) {
    clear();

    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);

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
        view.goto_recorded_handle(entities, collapser);
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
      case 'd':
        view.remove(entities, collapser, root_handles);
        break;
      default:
        // noop
        break;
    }
  }

  endwin();

  return 0;
}
