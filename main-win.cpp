#include "hierarchy/entity.hpp"

#include <wil/common.h>

#include <conio.h>
#include <cstdio>
#include <optional>
#include <stdlib.h>
#include <windows.h>

#define ESC "\x1b"
#define CSI "\x1b["

int main(int argc, char** argv) {
  // set input and ouput code pages
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);

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
  WI_ClearFlag(dwInMode, ENABLE_LINE_INPUT);
  WI_ClearFlag(dwInMode, ENABLE_ECHO_INPUT);

  if (!SetConsoleMode(hIn, dwInMode)) {
    // failed to set VT input mode, can't do anything here.
    return -1;
  }

  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);

  hy::interaction_t interaction;
  interaction.selected_ = root_handles.front();
  interaction.siblings_ = root_handles;

  const auto display_name = [](const hy::display_info_t& di) {
    printf(CSI "%d;%dH", di.level + 1, di.indent * 4); // set cursor position
    printf(
      di.last ? "\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80 "
              : "\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80 ");
    if (di.selected) {
      printf(CSI "7m"); // inverted
    }
    if (di.collapsed) {
      printf(CSI "1m"); // bold/bright
      printf(CSI "33m"); // foreground yellow
    }
    printf("%s\n", di.name.c_str());
    printf(CSI "0m"); // restore default
  };

  const auto display_connection = [](int level, int indent) {
    printf(CSI "%d;%dH", level + 1, indent * 4); // set cursor position
    printf("\xE2\x94\x82");
  };

  printf(CSI "?25l"); // hide cursor
  for (bool running = true; running;) {
    printf(CSI "1;1H"); // set cursor position
    printf(CSI "0J"); // clear screen

    // test to retrieve cursor position
    int r = 0;
    int c = 0;
    printf(CSI "6n");
    scanf(CSI "%d;%dR", &r, &c);

    hy::display_hierarchy(
      entities, interaction, root_handles, display_name, [] {},
      display_connection);

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
              return demo::input_e::collapse;
            case 72:
              return demo::input_e::move_up;
            case 77:
              return demo::input_e::expand;
            case 80:
              return demo::input_e::move_down;
            default:
              return std::nullopt;
          }
          break;
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
