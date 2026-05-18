#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 10000
#define MAX_LINE_LEN 4096
#define MODE_NORMAL 0
#define MODE_INSERT 1

static char *buf_lines[MAX_LINES];
static int buf_nlines = 0;
static int buf_nbytes = 0;
static char *filename = NULL;
static int cursor_row = 0;
static int cursor_col = 0;
static int mode = MODE_NORMAL;
static int startup_msg = 1;

static void buf_insert_char(char ch) {
  char *line = buf_lines[cursor_row];
  int len = strlen(line);
  char *new_line = malloc(len + 2);
  memcpy(new_line, line, cursor_col);
  new_line[cursor_col] = ch;
  memcpy(new_line + cursor_col + 1, line + cursor_col, len - cursor_col + 1);
  free(line);
  buf_lines[cursor_row] = new_line;
  cursor_col++;
}

static void buf_backspace(void) {
  if (cursor_col > 0) {
    char *line = buf_lines[cursor_row];
    memmove(line + cursor_col - 1, line + cursor_col,
            strlen(line) - cursor_col + 1);
    cursor_col--;
  } else if (cursor_row > 0) {
    char *prev = buf_lines[cursor_row - 1];
    char *cur = buf_lines[cursor_row];
    int prev_len = strlen(prev);
    int cur_len = strlen(cur);
    char *joined = malloc(prev_len + cur_len + 1);
    memcpy(joined, prev, prev_len);
    memcpy(joined + prev_len, cur, cur_len + 1);
    free(prev);
    free(cur);
    buf_lines[cursor_row - 1] = joined;
    memmove(buf_lines + cursor_row, buf_lines + cursor_row + 1,
            (buf_nlines - cursor_row - 1) * sizeof(char *));
    buf_nlines--;
    cursor_row--;
    cursor_col = prev_len;
  }
}

static void buf_split_line(void) {
  char *line = buf_lines[cursor_row];
  int len = strlen(line);
  int tail = len - cursor_col;
  char *new_line = malloc(tail + 1);
  memcpy(new_line, line + cursor_col, tail + 1);
  line[cursor_col] = '\0';
  memmove(buf_lines + cursor_row + 2, buf_lines + cursor_row + 1,
          (buf_nlines - cursor_row - 1) * sizeof(char *));
  buf_lines[cursor_row + 1] = new_line;
  buf_nlines++;
  cursor_row++;
  cursor_col = 0;
}

static void buf_insert_line(int row) {
  memmove(buf_lines + row + 1, buf_lines + row,
          (buf_nlines - row) * sizeof(char *));
  buf_lines[row] = strdup("");
  buf_nlines++;
}

static void format_position(char *pos, int pos_len) {
  if (buf_nlines == 0) {
    snprintf(pos, pos_len, "0,0-1");
    return;
  }
  int vim_line = cursor_row + 1;
  int line_len = strlen(buf_lines[cursor_row]);
  if (line_len == 0)
    snprintf(pos, pos_len, "%d,0-1", vim_line);
  else
    snprintf(pos, pos_len, "%d,%d", vim_line, cursor_col + 1);
}

static void draw(void) {
  int content_rows = LINES - 1;

  for (int r = 0; r < content_rows; r++) {
    move(r, 0);
    clrtoeol();
    if (r < buf_nlines) {
      mvprintw(r, 0, "%s", buf_lines[r]);
    } else if (buf_nlines > 0 || r > 0) {
      mvaddch(r, 0, '~');
    }
  }

  char pos[15];
  format_position(pos, sizeof(pos));
  char right[19];
  snprintf(right, sizeof(right), "%-14s%-4s", pos, "All");

  if (startup_msg) {
    char left[256];
    if (buf_nlines == 0)
      snprintf(left, sizeof(left), "\"%s\" 0L, 0B", filename);
    else
      snprintf(left, sizeof(left), "\"%s\" %dL, %dB", filename, buf_nlines,
               buf_nbytes);
    mvprintw(LINES - 1, 0, "%-62s%s", left, right);
  } else {
    mvprintw(LINES - 1, 0, "%-62s%s", "", right);
  }

  move(cursor_row, cursor_col);
  refresh();
}

static void handle_insert(int ch) {
  switch (ch) {
  case 27: // ESC
    mode = MODE_NORMAL;
    if (cursor_col > 0)
      cursor_col--;
    break;
  case KEY_BACKSPACE:
  case 127:
    buf_backspace();
    break;
  case '\r':
  case '\n':
    buf_split_line();
    break;
  default:
    if (ch >= 32 && ch < 127)
      buf_insert_char((char)ch);
    break;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  filename = argv[1];
  load_file(filename);

  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);

  draw();

  while (1)
    getch();

  endwin();
  return 0;
}
