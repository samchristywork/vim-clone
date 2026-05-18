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

static void load_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f)
    return;

  char line[MAX_LINE_LEN];
  while (fgets(line, sizeof(line), f) && buf_nlines < MAX_LINES) {
    int len = strlen(line);
    buf_nbytes += len;
    if (len > 0 && line[len - 1] == '\n')
      line[--len] = '\0';
    buf_lines[buf_nlines++] = strdup(line);
  }
  fclose(f);
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
    // else: empty file, row 0 stays as spaces
  }

  // Status line: left part + right ruler (always 18 chars)
  char left[256];
  if (buf_nlines == 0)
    snprintf(left, sizeof(left), "\"%s\" 0L, 0B", filename);
  else
    snprintf(left, sizeof(left), "\"%s\" %dL, %dB", filename, buf_nlines,
             buf_nbytes);

  char pos[15];
  if (buf_nlines == 0)
    snprintf(pos, sizeof(pos), "0,0-1");
  else
    snprintf(pos, sizeof(pos), "%d,%d", cursor_row + 1, cursor_col + 1);

  char right[19];
  snprintf(right, sizeof(right), "%-14s%-4s", pos, "All");

  mvprintw(LINES - 1, 0, "%-62s%s", left, right);

  move(cursor_row, cursor_col);
  refresh();
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
