#include "hierarchy/entity.hpp"

#include <locale.h>
#include <ncurses.h>
#include <optional>
#include <panel.h>
#include <stack>
#include <string.h>
#include <utility>

int main(int argc, char** argv) {
  thh::handle_vector_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);

  // enable support for unicode characters
  setlocale(LC_CTYPE, "");

  initscr(); // start curses mode
  cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  curs_set(0); // hide cursor

  hy::interaction_t interaction;
  interaction.select(root_handles.front(), entities, root_handles);

  const auto display_name = [](const hy::display_info_t& di) {
    mvprintw(
      di.level, di.indent * 4,
      di.last ? "\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80 "
              : "\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80 ");
    if (di.selected) {
      attron(A_REVERSE);
    }
    if (di.collapsed) {
      attron(A_BOLD);
    }
    printw("%s\n", di.name.c_str());
    attroff(A_REVERSE);
    attroff(A_BOLD);
  };

  const auto display_connection = [](int level, int indent) {
    mvprintw(level, indent * 4, "\xE2\x94\x82");
  };

  for (bool running = true; running;) {
    clear();

    hy::display_hierarchy(
      entities, interaction, root_handles, display_name, [] {},
      display_connection);

    refresh();
    move(0, 0);

    std::optional<demo::input_e> input = []() -> std::optional<demo::input_e> {
      switch (int key = getch(); key) {
        case KEY_LEFT:
          return demo::input_e::collapse;
        case KEY_RIGHT:
          return demo::input_e::expand;
        case KEY_UP:
          return demo::input_e::move_up;
        case KEY_DOWN:
          return demo::input_e::move_down;
        case 10: // enter
          return demo::input_e::add_child;
        default:
          return std::nullopt;
      }
    }();

    if (input.has_value()) {
      demo::process_input(input.value(), entities, root_handles, interaction);
    }
  }

  endwin();

  return 0;
}
