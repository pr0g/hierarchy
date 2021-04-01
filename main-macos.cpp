#include "hierarchy/entity.hpp"

#include <ncurses.h>
#include <optional>
#include <panel.h>
#include <stack>
#include <string.h>
#include <utility>

int main(int argc, char** argv) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);

  initscr(); // start curses mode
  cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  curs_set(0); // hide cursor

  hy::interaction_t interaction;
  interaction.selected_ = root_handles.front();
  interaction.neighbors_ = root_handles;

  const auto display_name = [](
                              int level, int indent, thh::handle_t /*unused*/,
                              bool selected, bool collapsed, bool has_children,
                              const std::string& name) {
    mvprintw(level, indent * 4, "|-- ");
    if (selected) {
      attron(A_REVERSE);
    }
    if (collapsed) {
      attron(A_BOLD);
    }
    printw("%s\n", name.c_str());
    attroff(A_REVERSE);
    attroff(A_BOLD);
  };

  const auto display_connection = [](int level, int indent) {
    mvprintw(level, indent * 4, "|");
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
          return demo::input_e::move_left;
        case KEY_RIGHT:
          return demo::input_e::move_right;
        case KEY_UP:
          return demo::input_e::move_up;
        case KEY_DOWN:
          return demo::input_e::move_down;
        case ' ':
          return demo::input_e::show_hide;
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
