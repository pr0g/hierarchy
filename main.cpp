#include <ncurses.h>
#include <panel.h>
#include <string.h>

#include "thh_handles/thh_handles.hpp"

#include <stack>
#include <string>
#include <utility>

struct entity_t {
  entity_t() = default;
  explicit entity_t(std::string name) : name_(std::move(name)) {}
  std::string name_;
  std::vector<thh::handle_t> children_;
  thh::handle_t parent_;
};

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
  auto handle_j = entities.add("entity_j");
  auto handle_k = entities.add("entity_k");

  auto* entity_a = entities.resolve(handle_a);
  auto* entity_b = entities.resolve(handle_b);
  auto* entity_c = entities.resolve(handle_c);
  auto* entity_d = entities.resolve(handle_d);
  auto* entity_e = entities.resolve(handle_e);
  auto* entity_f = entities.resolve(handle_f);
  auto* entity_g = entities.resolve(handle_g);
  auto* entity_h = entities.resolve(handle_h);
  auto* entity_i = entities.resolve(handle_i);
  auto* entity_j = entities.resolve(handle_j);
  auto* entity_k = entities.resolve(handle_k);

  entity_a->children_.push_back(handle_b);
  entity_a->children_.push_back(handle_c);
  entity_b->parent_ = handle_a;
  entity_c->parent_ = handle_a;

  entity_g->children_.push_back(handle_k);
  entity_k->parent_ = handle_g;

  entity_h->children_.push_back(handle_d);
  entity_h->children_.push_back(handle_e);
  entity_d->parent_ = handle_h;
  entity_e->parent_ = handle_h;

  entity_c->children_.push_back(handle_f);
  entity_c->children_.push_back(handle_g);
  entity_f->parent_ = handle_c;
  entity_g->parent_ = handle_c;

  entity_i->children_.push_back(handle_j);
  entity_j->parent_ = handle_i;

  initscr(); // start curses mode
  cbreak(); // line buffering disabled (respects Ctrl-C to quit)
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch
  curs_set(0); // hide cursor

  struct interaction_t {
    int element_ = 0;
    std::vector<thh::handle_t> neighbors_;

    thh::handle_t selected_;
    std::vector<thh::handle_t> collapsed_;

    bool is_collapsed(thh::handle_t handle) {
      return std::find(collapsed_.begin(), collapsed_.end(), handle)
          != collapsed_.end();
    }
  };

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

  interaction_t interaction;
  const auto display =
    [&entities, &interaction, &display_name,
     display_connection](const std::vector<thh::handle_t>& root_handles) {
      std::deque<thh::handle_t> entity_handle_stack;
      for (auto it = root_handles.rbegin(); it != root_handles.rend(); ++it) {
        entity_handle_stack.push_front(*it);
      }

      struct indent_tracker_t {
        int indent_ = 0;
        int count_ = 0;
      };

      std::deque<indent_tracker_t> indent_tracker;
      indent_tracker.push_front(
        indent_tracker_t{0, (int)entity_handle_stack.size()});

      const auto get_row_col = [] {
        int row = 0;
        int col = 0;
        getyx(stdscr, row, col);
        return std::pair(row, col);
      };

      const int indent_size = 4;
      while (!entity_handle_stack.empty()) {
        const auto curr_indent = indent_tracker.front().indent_;

        {
          auto& indent_ref = indent_tracker.front();
          indent_ref.count_--;
          if (indent_ref.count_ == 0) {
            indent_tracker.pop_front();
          }
        }

        auto entity_handle = entity_handle_stack.front();
        entity_handle_stack.pop_front();

        auto [row, col] = get_row_col();
        for (const auto ind : indent_tracker) {
          if (ind.count_ != 0 && ind.indent_ != curr_indent) {
            display_connection(row, ind.indent_);
          }
        }

        const auto* entity = entities.resolve(entity_handle);
        const auto selected = interaction.selected_ == entity_handle;
        display_name(
          row, col + curr_indent, selected,
          interaction.is_collapsed(entity_handle) && !entity->children_.empty(),
          entity->name_);

        const auto& children = entity->children_;
        if (!children.empty() && !interaction.is_collapsed(entity_handle)) {
          indent_tracker.push_front(
            indent_tracker_t{curr_indent + indent_size, (int)children.size()});
          for (auto it = children.rbegin(); it != children.rend(); ++it) {
            entity_handle_stack.push_front(*it);
          }
        }
      }
    };

  auto entity_handles = std::vector{handle_a, handle_h, handle_i};

  interaction.selected_ = handle_a;
  interaction.neighbors_ = entity_handles;

  for (bool running = true; running;) {
    clear();
    display(entity_handles);
    refresh();
    move(0, 0);
    int key = getch();
    if (key == KEY_RIGHT) {
      if (!interaction.is_collapsed(interaction.selected_)) {
        if (entity_t* entity = entities.resolve(interaction.selected_)) {
          if (!entity->children_.empty()) {
            interaction.selected_ = entity->children_.front();
            interaction.neighbors_ = entity->children_;
            interaction.element_ = 0;
            continue;
          }
        }
      }
    }
    if (key == KEY_LEFT) {
      if (entity_t* entity = entities.resolve(interaction.selected_)) {
        if (entity_t* parent = entities.resolve(entity->parent_)) {
          interaction.selected_ = entity->parent_;
          if (entity_t* grandparent = entities.resolve(parent->parent_)) {
            interaction.neighbors_ = grandparent->children_;
          } else {
            interaction.neighbors_ = entity_handles;
          }
          interaction.element_ =
            std::find(
              interaction.neighbors_.begin(), interaction.neighbors_.end(),
              interaction.selected_)
            - interaction.neighbors_.begin();
          continue;
        }
      }
    }
    if (key == KEY_DOWN) {
      interaction.element_ =
        (interaction.element_ + 1) % interaction.neighbors_.size();
      interaction.selected_ = interaction.neighbors_[interaction.element_];
    }
    if (key == KEY_UP) {
      interaction.element_ =
        (interaction.element_ + interaction.neighbors_.size() - 1)
        % interaction.neighbors_.size();
      interaction.selected_ = interaction.neighbors_[interaction.element_];
    }
    if (key == ' ') {
      if (interaction.is_collapsed(interaction.selected_)) {
        interaction.collapsed_.erase(
          std::remove(
            interaction.collapsed_.begin(), interaction.collapsed_.end(),
            interaction.selected_),
          interaction.collapsed_.end());
      } else {
        interaction.collapsed_.push_back(interaction.selected_);
      }
      continue;
    }
  }

  endwin();

  return 0;
}
