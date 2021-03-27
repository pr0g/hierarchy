#include "entity.hpp"

#include <stdlib.h>
#include <cstdio>
#include <windows.h>
#include <conio.h>

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

  const auto get_row_col = [&g_row, &g_col] {
    return std::pair(g_row, g_col);
  };

  printf(CSI "?25l"); // hide cursor
  for (bool running = true; running;) {
    g_row = 1;
    g_col = 1;
    printf(CSI "1;1H"); // set cursor position
    printf(CSI "0J"); // clear screen

    hy::display_hierarchy(
      entities, interaction, root_handles, display_name, display_connection,
      get_row_col);

    switch (int key = _getch(); key) {
      case 3:
        running = false;
        break;
      case 224:
      case 0:
        switch (int inner_key = _getch(); inner_key) {
          case 75:
            hy::try_move_left(interaction, entities, root_handles);
            break;
          case 72:
            hy::move_up(interaction);
            break;
          case 77:
            if (hy::try_move_right(interaction, entities)) {
              break;
            }
            [[fallthrough]];
          case 80:
            hy::move_down(interaction);
            break;
          default:
            break;
        }
      break;
      case ' ': // space
        hy::toggle_collapsed(interaction);
        break;
      default:
        break;
    }
  }

  printf(CSI "?25h"); // show cursor
  SetConsoleMode(hOut, dwOriginalOutMode);
  SetConsoleMode(hIn, dwOriginalInMode);

  return 0;
}
