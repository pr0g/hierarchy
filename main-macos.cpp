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
  // auto root_handles = demo::create_sample_entities(entities);
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

  hy::view_t view;
  view.offset_ = 0;
  view.count_ = 20;

  auto goto_handle = thh::handle_t();

  // builds everything
  auto flattened = hy::build_vector(entities, collapser, root_handles);

  for (bool running = true; running;) {
    clear();

    const int count = std::min((int)flattened.size(), view.offset_ + view.count_);
    for (int entity_index = view.offset_; entity_index < count; ++entity_index) {
      const auto& flatten = flattened[entity_index];
      if (entity_index == view.selected_) {
        attron(A_REVERSE);
      }
      if (collapser.collapsed(flatten.entity_handle_)) {
        attron(A_BOLD);
      }
      entities.call(flatten.entity_handle_, [&](const auto& entity) {
        mvprintw(
          entity_index - view.offset_, flatten.indent_ * 2,
          entity.name_.c_str());
      });
      attroff(A_REVERSE);
      attroff(A_BOLD);
    }

    refresh();
    move(0, 0);

    enum class input_e { up, down };

    switch (int key = getch(); key) {
      case KEY_UP:
        view.selected_ = std::max(view.selected_ - 1, 0);
        if (view.selected_ - view.offset_ + 1 == 0) {
          view.offset_ = std::max(view.offset_ - 1, 0);
        }
        break;
      case KEY_DOWN: {
        int temp = std::max((int)flattened.size() - view.count_, 0);
        view.selected_ = std::min(view.selected_ + 1, (int)flattened.size() - 1);
        if (view.selected_ - view.count_ - view.offset_ == 0) {
          view.offset_ = std::min(view.offset_ + 1, temp);
        }
      } break;
      case KEY_LEFT: {
        const auto entity_handle = flattened[view.selected_].entity_handle_;
        int count = hy::expanded_count(entity_handle, entities, collapser);
        collapser.collapse(entity_handle, entities);
        flattened.erase(
          flattened.begin() + 1 + view.selected_,
          flattened.begin() + view.selected_ + count);
      } break;
      case KEY_RIGHT: {
        const auto entity_handle = flattened[view.selected_].entity_handle_;
        if (collapser.collapsed(entity_handle)) {
          collapser.expand(entity_handle);
          auto handles = hy::build_hierarchy_single(
            entity_handle, flattened[view.selected_].indent_, entities, collapser);
          flattened.insert(
            flattened.begin() + view.selected_ + 1, handles.begin() + 1,
            handles.end());
        }
      } break;
      case 'g': {
        if (goto_handle != thh::handle_t()) {
          view.selected_ =
            hy::go_to_entity(goto_handle, entities, collapser, flattened);
          view.offset_ = view.selected_;
        }
      } break;
      case 'r': {
        goto_handle = flattened[view.selected_].entity_handle_;
      } break;
      case 'c': {
        if (!collapser.collapsed(flattened[view.selected_].entity_handle_)) {
          auto next_handle = entities.add();
          entities.call(next_handle, [next_handle](auto& entity) {
            entity.name_ =
              std::string("entity_") + std::to_string(next_handle.id_);
          });
          hy::add_children(
            flattened[view.selected_].entity_handle_, {next_handle}, entities);
          const auto child_count = hy::expanded_count(
            flattened[view.selected_].entity_handle_, entities, collapser);
          flattened.insert(
            flattened.begin() + view.selected_ + child_count - 1,
            {next_handle, flattened[view.selected_].indent_ + 1});
        }
      } break;
      case 's': {
        auto current_handle = flattened[view.selected_].entity_handle_;
        auto next_handle = entities.add();
        entities.call(next_handle, [next_handle](auto& entity) {
          entity.name_ =
            std::string("entity_") + std::to_string(next_handle.id_);
        });
        const auto siblings =
          hy::siblings(current_handle, entities, root_handles);

        int sibling_offset = 0;
        int siblings_left = 0;
        if (auto sibling_it =
              std::find(siblings.begin(), siblings.end(), current_handle);
            sibling_it != siblings.end()) {
          sibling_offset = sibling_it - siblings.begin();
          siblings_left = siblings.size() - sibling_offset;
        }

        int child_count = 0;
        for (int i = sibling_offset; i < siblings.size(); ++i) {
          child_count += hy::expanded_count(siblings[i], entities, collapser) - 1;
        }

        flattened.insert(
          flattened.begin() + view.selected_ + child_count + siblings_left,
          {next_handle, flattened[view.selected_].indent_});

        entities.call(current_handle, [&](hy::entity_t& entity) {
          auto parent_handle =
            entities
              .call_return(
                entity.parent_,
                [&](hy::entity_t& parent_entity) {
                  parent_entity.children_.push_back(next_handle);
                  return entity.parent_;
                })
              .value_or(thh::handle_t());
          if (parent_handle != thh::handle_t()) {
            entities.call(next_handle, [parent_handle](hy::entity_t& entity) {
              entity.parent_ = parent_handle;
            });
          } else {
            root_handles.push_back(next_handle);
          }
        });
      } break;
      default:
        // noop
        break;
    }

    // std::optional<demo::input_e> input = []() -> std::optional<demo::input_e>
    // {
    //   switch (int key = getch(); key) {
    //     case KEY_LEFT:
    //       return demo::input_e::collapse;
    //     case KEY_RIGHT:
    //       return demo::input_e::expand;
    //     case KEY_UP:
    //       return demo::input_e::move_up;
    //     case KEY_DOWN:
    //       return demo::input_e::move_down;
    //     case 10: // enter
    //       return demo::input_e::add_child;
    //     default:
    //       return std::nullopt;
    //   }
    // }();

    // if (input.has_value()) {
    //   demo::process_input(input.value(), entities, root_handles,
    //   interaction);
    // }
  }

  endwin();

  return 0;
}
