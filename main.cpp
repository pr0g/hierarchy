#include <ncurses.h>
#include <panel.h>
#include <string.h>

#include "thh_handles/thh_handles.hpp"

#include <stack>
#include <string>
#include <utility>

struct entity_t
{
  entity_t() = default;
  explicit entity_t(std::string name) : name_(std::move(name)) {}
  std::string name_;
  std::vector<thh::handle_t> children_;
};

int main(int argc, char** argv)
{
  thh::container_t<entity_t> entities;

  auto handle_a = entities.add("entity_a");
  auto handle_b = entities.add("entity_b");
  auto handle_c = entities.add("entity_c");
  auto handle_d = entities.add("entity_d");
  auto handle_e = entities.add("entity_e");
  auto handle_f = entities.add("entity_f");
  auto handle_g = entities.add("entity_g");
  auto handle_h = entities.add("entity_h");
  auto handle_i = entities.add("entity_i");
  auto handle_j = entities.add("entity_j");
  auto handle_k = entities.add("entity_k");

  auto* entity_a = entities.resolve(handle_a);
  entity_a->children_.push_back(handle_b);
  entity_a->children_.push_back(handle_c);

  auto* entity_g = entities.resolve(handle_g);
  entity_g->children_.push_back(handle_k);

  auto* entity_h = entities.resolve(handle_h);
  entity_h->children_.push_back(handle_d);
  entity_h->children_.push_back(handle_e);

  auto* entity_c = entities.resolve(handle_c);
  entity_c->children_.push_back(handle_f);
  entity_c->children_.push_back(handle_g);

  auto* entity_i = entities.resolve(handle_i);
  entity_i->children_.push_back(handle_j);

  initscr(); // start curses mode
  cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  curs_set(0); // hide cursor

  auto display = [&entities](const std::vector<thh::handle_t>& root_handles) {
    std::deque<thh::handle_t> entity_handle_stack;
    for (auto it = root_handles.rbegin(); it != root_handles.rend(); ++it) {
      entity_handle_stack.push_front(*it);
    }

    struct indent_tracker_t
    {
      int indent_ = 0;
      int count_ = 0;
    };

    const int indent_size = 3;
    std::deque<indent_tracker_t> indent_tracker;
    indent_tracker.push_front(indent_tracker_t{0, indent_size});

    const auto get_row_col = [] {
      int row;
      int col;
      getyx(stdscr, row, col);
      return std::pair(row, col);
    };

    while (!entity_handle_stack.empty()) {
      const auto curr_indent = indent_tracker.front().indent_;

      {
        auto& indenter_ref = indent_tracker.front();
        indenter_ref.count_--;
        if (indenter_ref.count_ == 0) {
          indent_tracker.pop_front();
        }
      }

      auto entity_handle = entity_handle_stack.front();
      entity_handle_stack.pop_front();

      auto [row, col] = get_row_col();
      for (const auto ind : indent_tracker) {
        if (ind.count_ != 0 && ind.indent_ != curr_indent) {
          mvprintw(row, ind.indent_, "|");
        }
      }

      const auto* entity = entities.resolve(entity_handle);

      mvprintw(row, col + curr_indent, "|-- ");
      printw("%s\n", entity->name_.c_str());

      const auto& children = entity->children_;
      if (!children.empty()) {
        indent_tracker.push_front(
          indent_tracker_t{curr_indent + indent_size, (int)children.size()});
      }
      for (auto it = children.rbegin(); it != children.rend(); ++it) {
        entity_handle_stack.push_front(*it);
      }
    }
  };

  display({handle_a, handle_h, handle_i});

  refresh();
  getch();
  endwin();

  return 0;
}

/*
  initscr(); // start curses mode
  raw(); // line buffering disabled (ignores Ctrl-C or Ctrl-Z to quit)
  // cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  // nodelay(stdscr, true);
  // halfdelay(10);

  printw("Type a character to see it in bold\n");
  int ch = getch();
  printw("The key pressed is: ");
  attron(A_BOLD);
  printw("%c\n", ch);
  attroff(A_BOLD);

  addch('t' | A_BOLD | A_UNDERLINE);
  addch('\n');

  const char* msg = "Hello, Tom!";
  int row;
  int col;
  getmaxyx(stdscr, row, col);
  mvprintw(row/2, (col - strlen(msg))/2, "%s", msg);
  mvprintw(row-2,0,"This screen has %d rows and %d columns\n",row,col);
  printw("Try resizing your window(if possible) and then run this program
  again");

  refresh();
  getch();
  endwin();
*/
