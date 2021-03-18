#include <ncurses.h>

int main(int argc, char** argv) {
  initscr(); // start curses mode
  raw(); // line buffering disabled
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch

  printw("Type a character to see it in bold\n");
  int ch = getch();
  printw("The key pressed is: ");
  attron(A_BOLD);
  printw("%c", ch);
  attroff(A_BOLD);

  refresh();
  getch();
  endwin();

  return 0;
}
