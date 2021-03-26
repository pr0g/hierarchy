#include "entity.hpp"

#include <ncurses.h>
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

  const auto display_name = [](
                              int row, int col, bool selected,
                              bool hidden_children, const std::string& name) {
    mvprintw(row, col, "|-- ");
    if (selected) {
      attron(A_REVERSE);
    }
    if (hidden_children) {
      attron(A_BOLD);
    }
    printw("%s\n", name.c_str());
    attroff(A_REVERSE);
    attroff(A_BOLD);
  };

  const auto display_connection = [](int row, int col) {
    mvprintw(row, col, "|");
  };

  const auto get_row_col = [] {
    int row = 0;
    int col = 0;
    getyx(stdscr, row, col);
    return std::pair(row, col);
  };

  hy::interaction_t interaction;
  interaction.selected_ = root_handles.front();
  interaction.neighbors_ = root_handles;

  for (bool running = true; running;) {
    clear();
    hy::display_hierarchy(
      entities, interaction, root_handles, display_name, display_connection,
      get_row_col);
    refresh();
    move(0, 0);
    switch (int key = getch(); key) {
      case KEY_RIGHT:
        hy::try_move_right(interaction, entities);
        break;
      case KEY_LEFT:
        hy::try_move_left(interaction, entities, root_handles);
        break;
      case KEY_DOWN:
        hy::move_down(interaction);
        break;
      case KEY_UP:
        hy::move_up(interaction);
        break;
      case ' ':
        hy::toggle_collapsed(interaction);
        break;
      default:
        break;
    }
  }

  endwin();

  return 0;
}
