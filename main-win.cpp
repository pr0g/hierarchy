#include "hierarchy/entity.hpp"

#include <conio.h>
#include <cstdio>
#include <stdlib.h>
#include <windows.h>
#include <optional>

#define ESC "\x1b"
#define CSI "\x1b["

int main(int argc, char** argv) {
  // set output mode to handle virtual terminal sequences
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) {
    return 1;
  }
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  if (hIn == INVALID_HANDLE_VALUE) {
    return 1;
  }

  DWORD dwOriginalOutMode = 0;
  DWORD dwOriginalInMode = 0;
  if (!GetConsoleMode(hOut, &dwOriginalOutMode)) {
    return 1;
  }
  if (!GetConsoleMode(hIn, &dwOriginalInMode)) {
    return 1;
  }

  DWORD dwRequestedOutModes =
    ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;

  DWORD dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
  if (!SetConsoleMode(hOut, dwOutMode)) {
    // we failed to set both modes, try to step down mode gracefully.
    dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
    if (!SetConsoleMode(hOut, dwOutMode)) {
      // failed to set any VT mode, can't do anything here.
      return -1;
    }
  }

  DWORD dwInMode = dwOriginalInMode | ENABLE_VIRTUAL_TERMINAL_INPUT;
  if (!SetConsoleMode(hIn, dwInMode)) {
    // failed to set VT input mode, can't do anything here.
    return -1;
  }

  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);

  hy::interaction_t interaction;
  interaction.selected_ = root_handles.front();
  interaction.neighbors_ = root_handles;

  int g_row = 1;
  int g_col = 1;

  const auto display_name = [&g_row, &g_col](
                              int row, int col, bool selected,
                              bool hidden_children, const std::string& name) {
    g_row = row + 1;
    g_col = 1;
    printf(CSI "%d;%dH", row, col); // set cursor position
    printf("|-- ");
    if (selected) {
      printf(CSI "7m"); // inverted
    }
    if (hidden_children) {
      printf(CSI "1m"); // bold/bright
      printf(CSI "33m"); // foreground yellow
    }
    printf("%s\n", name.c_str());
    printf(CSI "0m"); // restore default
  };

  const auto display_connection = [](int row, int col) {
    printf(CSI "%d;%dH", row, col); // set cursor position
    printf("|");
  };

  const auto get_row_col = [&g_row, &g_col] { return std::pair(g_row, g_col); };

  printf(CSI "?25l"); // hide cursor
  for (bool running = true; running;) {
    g_row = 1;
    g_col = 1;
    printf(CSI "1;1H"); // set cursor position
    printf(CSI "0J"); // clear screen

    hy::display_hierarchy(
      entities, interaction, root_handles, display_name, display_connection,
      get_row_col);

    std::optional<demo::input_e> input =
      [&running]() -> std::optional<demo::input_e> {
      switch (int key = _getch(); key) {
        case 3:
          running = false;
          return std::nullopt;
        case 224:
        case 0:
          switch (int inner_key = _getch(); inner_key) {
            case 75:
              return demo::input_e::move_left;
            case 72:
              return demo::input_e::move_up;
            case 77:
              return demo::input_e::move_right;
            case 80:
              return demo::input_e::move_down;
            default:
              return std::nullopt;
          }
          break;
        case ' ': // space
          return demo::input_e::show_hide;
        case 13: // enter
          return demo::input_e::add_child;
        default:
          return std::nullopt;
      }
    }();

    if (input.has_value()) {
      demo::process_input(input.value(), entities, root_handles, interaction);
    }
  }

  printf(CSI "?25h"); // show cursor
  SetConsoleMode(hOut, dwOriginalOutMode);
  SetConsoleMode(hIn, dwOriginalInMode);

  return 0;
}
