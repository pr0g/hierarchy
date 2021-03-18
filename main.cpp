#include <ncurses.h>
#include <string.h>

int main(int argc, char** argv) {
  initscr(); // start curses mode
  raw(); // line buffering disabled
  keypad(stdscr, true); // enable function keys
  noecho(); // don't echo while we do getch

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

  return 0;
}
