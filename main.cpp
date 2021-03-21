#include <ncurses.h>
#include <panel.h>
#include <string.h>

///
#include "thh_handles/thh_handles.hpp"

#include <string>
#include <utility>
#include <stack>

struct entity_t {
  entity_t() = default;
  explicit entity_t(std::string  name) : name_(std::move(name)) {}
  std::string name_;
  std::vector<thh::handle_t> children_;
};

///

int main(int argc, char** argv) {
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

  auto* entity_a = entities.resolve(handle_a);
  entity_a->children_.push_back(handle_b);
  entity_a->children_.push_back(handle_c);

  auto* entity_b = entities.resolve(handle_b);
  entity_b->children_.push_back(handle_d);
  entity_b->children_.push_back(handle_e);

  auto* entity_c = entities.resolve(handle_c);
  entity_c->children_.push_back(handle_f);
  entity_c->children_.push_back(handle_g);

  auto display = [&entities](const std::vector<thh::handle_t>& root_handles) {
    initscr(); // start curses mode
    cbreak(); // line buffering disabled (respects Ctrl-C to quit)
    keypad(stdscr, true); // enable function keys
    noecho(); // don't echo while we do getch
    curs_set(0); // hide cursor

    std::stack<thh::handle_t> entity_handle_stack;
    // for (const auto handle : root_handles) {
    for (auto rev_it = root_handles.rbegin(); rev_it != root_handles.rend(); ++rev_it) {
      entity_handle_stack.push(*rev_it);
    }

    struct indent_tracker_t {
      int indent_ = 0;
      int count_ = 0;
    };

    std::stack<indent_tracker_t> indent_tracker;
    for (int i = 0; i < root_handles.size(); ++i) {
      indent_tracker.push(indent_tracker_t{0, 1});
    }

    while (!entity_handle_stack.empty()) {
      auto& idt = indent_tracker.top();
      auto curr_indent = idt.indent_;
      idt.count_--;
      if (idt.count_ == 0) {
        indent_tracker.pop();
      }

      auto top = entity_handle_stack.top();
      entity_handle_stack.pop();
      const auto* e = entities.resolve(top);
      int row;
      int col;
      getyx(stdscr, row, col);
      mvprintw(row, col + curr_indent, "%s\n", e->name_.c_str());
      if (!e->children_.empty()) {
        indent_tracker.push(indent_tracker_t{curr_indent + 1, (int)e->children_.size()});
      }
      for (auto rev_it = e->children_.rbegin(); rev_it != e->children_.rend(); ++rev_it) {
        entity_handle_stack.push(*rev_it);
      }
    }

    refresh();
    getch();
    endwin();
  };

   display({handle_a, handle_h, handle_i});

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
  printw("Try resizing your window(if possible) and then run this program again");

  refresh();
  getch();
  endwin();
*/
