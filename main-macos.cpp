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

  hy::view_t view;
  view.offset = 0;
  view.count = 7;

  int selected = 0;

  // builds everything
  auto flattened = hy::build_vector(entities, view, interaction, root_handles);

  for (bool running = true; running;) {
    clear();

    const int count = std::min((int)flattened.size(), view.offset + view.count);
    for (int entity_index = view.offset; entity_index < count; ++entity_index) {
      const auto& flatten = flattened[entity_index];
      if (entity_index == selected) {
        attron(A_REVERSE);
      }
      if (interaction.collapsed(flatten.entity_handle_)) {
        attron(A_BOLD);
      }
      entities.call(flatten.entity_handle_, [&](const auto& entity) {
        mvprintw(
          entity_index - view.offset, flatten.indent_ * 4,
          entity.name_.c_str());
      });
      attroff(A_REVERSE);
      attroff(A_BOLD);
    }

    // hy::display_hierarchy(
    //   entities, view, interaction, root_handles, display_name, [] {},
    //   display_connection);

    refresh();
    move(0, 0);

    enum class input_e { up, down };

    switch (int key = getch(); key) {
      case KEY_UP:
        selected = std::max(selected - 1, 0);
        if (selected - view.offset + 1 == 0) {
          view.offset = std::max(view.offset - 1, 0);
        }
        break;
      case KEY_DOWN: {
        int temp = std::max((int)flattened.size() - view.count, 0);
        selected = std::min(selected + 1, (int)flattened.size() - 1);
        if (selected - view.count - view.offset == 0) {
          view.offset = std::min(view.offset + 1, temp);
        }
      } break;
      case KEY_LEFT: {
        const auto entity_handle = flattened[selected].entity_handle_;
        int count = hy::expanded_count_again(entity_handle, entities, interaction);
        interaction.collapse(entity_handle, entities);
        flattened.erase(
          flattened.begin() + 1 + selected,
          flattened.begin() + selected + count);
      } break;
      case KEY_RIGHT: {
        const auto entity_handle = flattened[selected].entity_handle_;
        if (interaction.collapsed(entity_handle)) {
          interaction.expand(entity_handle);
          auto handles = hy::build_hierarchy_single(
            entity_handle, flattened[selected].indent_, entities, interaction);
          flattened.insert(
            flattened.begin() + selected + 1, handles.begin() + 1,
            handles.end());
        }
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
