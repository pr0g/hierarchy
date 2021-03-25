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
  const auto display =
    [&entities, &interaction, &display_name, display_connection,
     &get_row_col](const std::vector<thh::handle_t>& root_handles) {
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

  interaction.selected_ = root_handles.front();
  interaction.neighbors_ = root_handles;

  for (bool running = true; running;) {
    clear();
    display(root_handles);
    refresh();
    move(0, 0);
    int key = getch();
    if (key == KEY_RIGHT) {
      if (!interaction.is_collapsed(interaction.selected_)) {
        if (hy::entity_t* entity = entities.resolve(interaction.selected_)) {
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
      if (hy::entity_t* entity = entities.resolve(interaction.selected_)) {
        if (hy::entity_t* parent = entities.resolve(entity->parent_)) {
          interaction.selected_ = entity->parent_;
          if (hy::entity_t* grandparent = entities.resolve(parent->parent_)) {
            interaction.neighbors_ = grandparent->children_;
          } else {
            interaction.neighbors_ = root_handles;
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
