// 
// turbougly
// The naive CSS minifer
//
// Copyright (c) 2012 Wander Nauta/The WanderNauta Company

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// This program's entry point, main(), is at the bottom of the file.

// -- Header includes --

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

// -- Static, global flags --

static int verbose = 0;
static int summary = 0;
static int help = 0;
static int version = 0;

// -- Constants --

#define PROGRAM "turbougly"
#define MAJOR_V 0
#define MINOR_V 1

// -- Utility function prototypes --

char* shuffle(char*, unsigned int);
void mark(unsigned int, char*, unsigned int);
bool ishexstr(char*);
void error(int, int, char*);

// -- Utility functions --

// Shuffle the buffer to put all zero bytes at the end
char* shuffle(char* old, unsigned int bufsz) {
  char* new = (char*)calloc(bufsz, 1);
  if (!new) { exit(EXIT_FAILURE); return "\0"; } // Bail!
  char* from = old;
  char* to = new;
  char* end = old + bufsz;

  while (1) {
    while (*from == '\0' && from != end) from++;
    if (from == end) break;

    unsigned int nonnull_bytes = strlen(from);
    memcpy(to, from, nonnull_bytes);
    to += nonnull_bytes;
    from += nonnull_bytes;
  }

  free(old);
  return new;
}

// Output some statistics to stderr
void mark(unsigned int filter, char* buf, unsigned int bufsz) {
  unsigned int buflen = bufsz - 1;
  unsigned int len = strlen(buf);
  unsigned int savings = buflen - len;
  const int maxw = 50;
  int width = (int)(((float)len / buflen) * maxw);
  fprintf(stderr, "Filt %u: -%-8u [", filter, savings);
  for (int i = 0; i < width; i++) fprintf(stderr, "#");
  for (int i = 0; i < (maxw - width); i++) fprintf(stderr, " ");
  fprintf(stderr, "] t=%ld\n", clock());
}

// Check if the given string starts with exactly 6 hex chars (0-9 a-f A-F)
inline bool ishexstr(char* c) {
  return (strspn(c, "1234567890ABCDEFabcdef") == 6);
}

// Display an error, and optionally exit
void error(int status, int errno, char* msg) {
  fprintf(stderr, "turbougly:");
  if (msg[0] != '\0') fprintf(stderr, " %s", msg);
  if (errno != 0) fprintf(stderr, ": %s", strerror(errno));
  fprintf(stderr, "\n");
  if (status != 0) exit(status);
}

// -- Obliterate tabs, newlines and repeated spaces --
bool clean_spaces(char* buf) {
  while (1) {
    buf = strpbrk(buf, "\t\n ");
    if (buf == NULL) break;

    if (*buf == '\t') *buf = '\0';
    else if (*buf == '\n') *buf = '\0';
    else if (*buf == ' ' && *(buf-1) == ' ')  *(buf-1) = '\0';

    buf++;
  }

  return true;
}

// --  Strip comments --

bool strip_comments(char* buf) {
  char* i = buf;
  bool modified = false;

  while (1) {
    char* begin = strstr(i, "/*");
    if (begin == NULL) break;
    char* end = strstr(begin + 2, "*/") + 2;
    size_t len = (size_t)(end - begin);
    memset(begin, 0, len);
    modified = true;
    i = end;
  }

  return modified;
}

// -- Zealously collapse whitespace --

bool strip_white(char* buf) {
  unsigned int i = 0;
  bool modified = false;

  while (1) {
    // Find the next brace, colon, semicolon or comma
    unsigned int symbol_dist = strcspn(buf + i, "{:;,");
    unsigned int symbol_i = i + symbol_dist;

    if (buf[symbol_i] == '\0') break; // We reached end-of-string

    // Count the amount of spaces that follow and clear them
    unsigned int post = strspn(buf + symbol_i + 1, "   ");
    memset(buf + symbol_i + 1, 0, post);

    // Count the amount of spaces that precede and clear them
    unsigned int pre = 0;
    if (buf[symbol_i] != ':') {
      unsigned int j = symbol_i - 1;
      while (j > 0 && buf[j] == ' ') { j--; pre++; }
      memset(buf + j + 1, 0, pre);
    }

    if (pre > 0 || post > 0) modified = true;

    // Move past the cleared bit
    i += symbol_dist + post + 1;
  }

  return modified;
}

// -- Collapse color functions --

bool collapse_funcs(char* buf) {
  bool modified = false;
  char* i = buf;
  char* start = buf;

  while (1) {
    i = strstr(i, "rgb(");
    if (i == NULL) break;

    start = i;
    int r = atoi(i + 4); while (*i != ',') i++;
    int g = atoi(i); while (*i != ',') i++;
    int b = atoi(i); while (*i != ')') i++;
    i++; // Eat last paren

    // Clear the part
    memset(start, 0, i - start);

    // Build up the hex value
    *start = '#';
    snprintf(start + 1, 3, "%02x", r);
    snprintf(start + 3, 3, "%02x", g);
    snprintf(start + 5, 3, "%02x", b);

    modified = true;
  }

  return modified;
}

// -- Collapse hex values --

bool collapse_hex(char* buf) {
  bool modified = false;
  char* i = buf;

  while (1) {
    i = strchr(i, '#');
    if (i == NULL) break;

    i += 7;

    if (ishexstr(i-6)) {
      if (*(i-6) == *(i-5) && *(i-4) == *(i-3) && *(i-2) == *(i-1)) {
        *(i-5) = *(i-4);
        *(i-4) = *(i-2);
        
        memset(i-3, 0, 3);
        modified = true;
      }
    }
  }

  return modified;
}

// -- Collapse zero values --

bool collapse_zero(char* buf) {
  bool modified = false;
  char* i = buf;

  while (1) {
    i = strchr(i, '.');
    if (i == NULL) break;

    if (*(i-2) == ':' && *(i-1) == '0') {
      *(i-1) = '\0';
      modified = true;
    }

    i++;
  }
  return modified;
}

// -- Collapse unneeded semicolons --

bool collapse_semi(char* buf) {
  bool modified = false;
  char* i = buf;

  while (1) {
    i = strchr(i, ';');
    if (i == NULL) break;

    if (*(i-1) == ';') {
      *i = '\0';
      modified = true;
    }

    if (*(i+1) == '}') {
      *i = '\0';
      modified = true;
    }

    i++;
  }

  return modified;
}

// -- Collapse empty declarations --

bool strip_empty_decl(char* buf) {
  bool modified = false;
  char* i = buf;
  char* j = buf;

  while (1) {
    i = strstr(i, "{}");
    if (i == NULL) break;

    i++;
    *(i) = '\0'; // Cut the string at i

    j = strrchr(buf, '}');
    memset(j + 1, 0, i - j);
    modified = true;
  }

  return modified;
}

// -- Runner --

int main(int argc, char* argv[]) {
  // Parse command line arguments
  int c = 0, option_index = 0;

  static struct option long_options[] = {
    {"help",    no_argument, &help,    1},
    {"summary", no_argument, &summary, 1},
    {"usage",   no_argument, &help,    1},
    {"verbose", no_argument, &verbose, 1},
    {"version", no_argument, &version, 1},
    {0, 0, 0, 0}
  };

  while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
    // Do nothing. Getopt handles long option flags.
  }

  if (help || version) printf("%s v%d.%d\n", PROGRAM, MAJOR_V, MINOR_V);
  if (help)            printf("Usage: %s [--verbose|--summary] <file>\n", argv[0]);
  if (version)         printf("Copyright (c) 2012 Wander Nauta/The WanderNauta Company\n");

  if (help) printf("\n    Report bugs to: <https://github.com/wandernauta/%s>\n\n", PROGRAM);
  if (help || version) exit(0);

  // Read and measure the file
  FILE* fd;
  fd = fopen(argv[optind], "r");
  if (fd == NULL) error(1, 0, "Couldn't open file.");
  fseek(fd, 0L, SEEK_END);
  unsigned int bufsz = (unsigned int)ftell(fd) + 1;
  fseek(fd, 0L, SEEK_SET);

  // Allocate, initialize and fill a large enough buffer (plus padding)
  char* buf = (char*)calloc(bufsz, 1);
  if (!buf) exit(EXIT_FAILURE); // Whoa!
  fread(buf, bufsz, 1, fd);
  fclose(fd);

  // Run the filters in succession
  if (verbose) fprintf(stderr, "Size report for %s:\n", argv[optind]);
  if (verbose) mark(0, buf, bufsz);
  if (clean_spaces(buf))     buf = shuffle(buf, bufsz); if (verbose) mark(1, buf, bufsz);
  if (strip_comments(buf))   buf = shuffle(buf, bufsz); if (verbose) mark(2, buf, bufsz);
  if (strip_white(buf))      buf = shuffle(buf, bufsz); if (verbose) mark(3, buf, bufsz);
  if (collapse_funcs(buf))   buf = shuffle(buf, bufsz); if (verbose) mark(4, buf, bufsz);
  if (collapse_hex(buf))     buf = shuffle(buf, bufsz); if (verbose) mark(5, buf, bufsz);
  if (collapse_zero(buf))    buf = shuffle(buf, bufsz); if (verbose) mark(6, buf, bufsz);
  if (collapse_semi(buf))    buf = shuffle(buf, bufsz); if (verbose) mark(7, buf, bufsz);
  if (strip_empty_decl(buf)) buf = shuffle(buf, bufsz); if (verbose) mark(8, buf, bufsz);

  if (verbose || summary) fprintf(stderr, "Old size: %u bytes - New size: %zd bytes - Diff: -%zu bytes (-%.0f%%)\n", bufsz - 1, strlen(buf), ((bufsz-1) - strlen(buf)), ((((bufsz-1.0) - strlen(buf)) / (bufsz-1.0)) * 100.0));

  // Print the result
  fputs(buf, stdout);

  // Clean up
  free(buf);
}
