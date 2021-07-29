#include "hierarchy/entity.hpp"

#include <algorithm>
#include <locale.h>
#include <ncurses.h>
#include <optional>
#include <panel.h>
#include <stack>
#include <string.h>
#include <utility>

int main(int argc, char** argv) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);

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

  hy::view_t view(hy::flatten_entities(entities, collapser, root_handles), 0, 20);

  for (bool running = true; running;) {
    clear();

    int min_indent = std::numeric_limits<int>::max();
    thh::handle_t min_indent_handle;

    std::vector<std::pair<int, int>> connections;
    const auto min_elements = std::min((int)view.flattened_handles().size(), view.count());
    const int size = view.flattened_handles().size();
    // find another matching indent before a lower indent is found
    std::vector<bool> ends;
    // search 'upwards' first (reverse iterators)
    for (int indent_index = 0; indent_index < min_elements; ++indent_index) {
      if (view.flattened_handles()[indent_index + view.offset()].indent_ < min_indent) {
        min_indent = view.flattened_handles()[indent_index + view.offset()].indent_;
        min_indent_handle = view.flattened_handles()[indent_index + view.offset()].entity_handle_;
      }
      int prev_indent_index = indent_index - 1;
      if (auto rfound = std::find_if(
            view.flattened_handles().rbegin() + (size - 1) - (view.offset() + prev_indent_index),
            view.flattened_handles().rend(),
            [&view, indent_index](const auto& flattened_handle) {
              return view.flattened_handles()[indent_index + view.offset()].indent_
                  == flattened_handle.indent_;
            });
          rfound != view.flattened_handles().rend()) {
        auto ffound = (rfound + 1).base();
        auto dist = ffound - view.flattened_handles().begin();
        if (dist < view.offset()) {
          if (std::all_of(
                view.flattened_handles().rbegin() + (size - 1)
                  - (view.offset() + prev_indent_index),
                rfound, [&view, indent_index](const auto& flattened_handle) {
                  return flattened_handle.indent_
                      >= view.flattened_handles()[indent_index + view.offset()].indent_;
                })) {
            int range = rfound
                      - (view.flattened_handles().rbegin() + (size - 1)
                         - (view.offset() + prev_indent_index));
            for (int i = 0; i < range; ++i) {
              int is = prev_indent_index - i;
              if (is < 0) {
                break;
              }
              connections.push_back(
                {view.flattened_handles()[indent_index + view.offset()].indent_, is});
            }
          }
        }
      }
      int next_indent_index = indent_index + 1;
      if (auto found = std::find_if(
            view.flattened_handles().begin() + view.offset() + next_indent_index,
            view.flattened_handles().end(),
            [&view, indent_index](const auto& flattened_handle) {
              return view.flattened_handles()[indent_index + view.offset()].indent_
                  == flattened_handle.indent_;
            });
          found != view.flattened_handles().end()) {
        if (std::all_of(
              view.flattened_handles().begin() + next_indent_index + view.offset(), found,
              [&view, indent_index](const auto& flattened_handle) {
                return flattened_handle.indent_
                    >= view.flattened_handles()[indent_index + view.offset()].indent_;
              })) {
          int range =
            found - (view.flattened_handles().begin() + view.offset() + next_indent_index);
          for (int i = 0; i < range; i++) {
            int is = next_indent_index + i;
            int min = std::min(view.count(), (int)view.flattened_handles().size() - view.offset());
            if (is >= min) {
              break;
            }
            connections.push_back(
              {view.flattened_handles()[indent_index + view.offset()].indent_, is});
          }
          ends.push_back(false);
        } else {
          ends.push_back(true);
        }
      } else {
        ends.push_back(true);
      }
    }

    // draw all lines for entities that are offscreen
    for (int i = min_indent - 1; i >= 0; --i) {
      bool draw = entities
                    .call_return(
                      min_indent_handle,
                      [&](const hy::entity_t& entity) {
                        // update min indent as we walk backwards right to left
                        min_indent_handle = entity.parent_;
                        return entities
                          .call_return(
                            entity.parent_,
                            [&](const hy::entity_t& parent) {
                              if (parent.parent_ == thh::handle_t()) {
                                if (auto it = std::find(
                                      root_handles.begin(), root_handles.end(), entity.parent_);
                                    it != root_handles.end()) {
                                  auto position = it - root_handles.begin();
                                  return position != (root_handles.size() - 1);
                                }
                                return false;
                              }
                              return entities
                                .call_return(
                                  parent.parent_,
                                  [&](const hy::entity_t& grandparent) {
                                    if (auto it = std::find(
                                          grandparent.children_.begin(),
                                          grandparent.children_.end(), entity.parent_);
                                        it != grandparent.children_.end()) {
                                      auto position = it - grandparent.children_.begin();
                                      return position != (grandparent.children_.size() - 1);
                                    }
                                    return false;
                                  })
                                .value_or(false);
                            })
                          .value_or(false);
                      })
                    .value_or(false);
      if (draw) {
        for (int r = 0; r < view.count(); r++) {
          mvprintw(r, i * 4, "\xE2\x94\x82");
        }
      }
    }

    assert(ends.size() == std::min((int)view.flattened_handles().size(), view.count()));

    for (const auto& connection : connections) {
      mvprintw(connection.second, connection.first * 4, "\xE2\x94\x82");
    }

    const int count = std::min((int)view.flattened_handles().size(), view.offset() + view.count());
    for (int handle_index = view.offset(); handle_index < count; ++handle_index) {
      const auto& flattened_handle = view.flattened_handles()[handle_index];

      mvprintw(
        handle_index - view.offset(), flattened_handle.indent_ * 4,
        ends[handle_index - view.offset()] ? "\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80 "
                                           : "\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80 ");

      if (handle_index == view.selected()) {
        attron(A_REVERSE);
      }
      if (collapser.collapsed(flattened_handle.entity_handle_)) {
        attron(A_BOLD);
      }
      entities.call(flattened_handle.entity_handle_, [&](const auto& entity) {
        printw("%s\n", entity.name_.c_str());
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
