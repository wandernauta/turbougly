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

// -- Utility function prototypes --

void shuffle(char*, unsigned int);
void s(char*, unsigned int);
bool space(char);
bool hex(char);
void error(int, int, char*);

// -- Utility functions --

// Shuffle the buffer to put all zero bytes at the end
void shuffle(char* buf, unsigned int bufsz) {
  unsigned int from = 0, to = 0;

  while (from < bufsz) {
    if (buf[from] == '\0') {
      from++;
    } else {
      buf[to] = buf[from];
      from++;
      to++;
    }
  }

  memset(buf + to, 0, from - to); // Clear the tail
}

// Set the byte pointed to by i to zero
inline void s(char* buf, unsigned int i) {
  memset(buf + i, 0, 1);
}

// Check if the given char is a space, tab or newline
inline bool space(char c) {
  return (c == ' ' || c == '\t' || c == '\n');
}

// Check if the given char is a hexadecimal (0-9 a-f A-F)
inline bool hex(char c) {
  return isxdigit(c);
}

// Display an error, and optionally exit
void error(int status, int errno, char* msg) {
  fprintf(stderr, "turbougly:");
  if (msg[0] != '\0') fprintf(stderr, " %s", msg);
  if (errno != 0) fprintf(stderr, ": %s", strerror(errno));
  fprintf(stderr, "\n");
  if (status != 0) exit(status);
}

// -- Phase function prototypes --

bool p0(char*, unsigned int);
bool p1(char*, unsigned int);
bool p2(char*, unsigned int);
bool p3(char*, unsigned int);
bool p4(char*, unsigned int);
bool p5(char*, unsigned int);
bool p6(char*, unsigned int);
bool p7(char*, unsigned int);

// -- Phase zero: obliterate tabs, newlines and repeated spaces -- 

bool p0(char* buf, unsigned int bufsz) {
  for (unsigned int i = 0; i <= bufsz; ++i) {
    if (buf[i] == '\t') {
      s(buf, i);
      i--;
    } else if (buf[i] == '\n') {
      s(buf, i);
      i--;
    } else if (i > 0 && (buf[i-1] == ' ' || buf[i-1] == '\0') && buf[i] == ' ') {
      memset(buf + i, 0, strspn(buf + i, "   "));
    }
  }

  return true;
}

// -- Phase one: strip comments --

bool p1(char* buf, unsigned int bufsz) {
  bool in_comment = false;

  for (unsigned int i = 0; i < (bufsz - 1); ++i) {
    if (buf[i] == '/' && buf[i+1] == '*') {
      in_comment = true;
      s(buf, i);
      continue;
    } else if (buf[i] == '*' && buf[i+1] == '/') {
      in_comment = false;
      s(buf, i);
      s(buf, i+1);
      continue;
    } else {
      if (in_comment) s(buf, i);
    }
  }

  return true;
}

// -- Phase two: zealously collapse whitespace --

bool p2(char* buf, unsigned int bufsz) {
  unsigned int i = 0;

  while (i < bufsz) {
    // Find the next brace, colon, semicolon or comma
    unsigned int symbol_dist = strcspn(buf + i, "{:;,");
    unsigned int symbol_i = i + symbol_dist;

    if (buf[symbol_i] == '\0') break; // We reached end-of-string

    // Count the amount of spaces that follow and clear them
    unsigned int post = strspn(buf + symbol_i + 1, "   ");
    memset(buf + symbol_i + 1, 0, post);

    // Count the amount of spaces that precede and clear them
    if (buf[symbol_i] != ':') {
      unsigned int pre = 0;
      unsigned int j = symbol_i - 1;
      while (j > 0 && buf[j] == ' ') { j--; pre++; }
      memset(buf + j + 1, 0, pre);
    }

    // Move past the cleared bit
    i += symbol_dist + post + 1;
  }

  return true;
}

// -- Phase three: collapse color function --

bool p3(char* buf, unsigned int bufsz) {
  for (unsigned int i = 0; i < bufsz; i++) {
    if (memcmp(buf + i, "rgb(", 4) == 0) {
      unsigned int start = i;
      int r = atoi(buf + i + 4);
      while (buf[i] != ',') i++;
      int g = atoi(buf + i);
      while (buf[i] != ',') i++;
      int b = atoi(buf + i);
      while (buf[i] != ')') i++;
      i++; // Eat last paren
      memset(buf + start, 0, i - start);

      // Build up the hex value
      buf[start] = '#';
      snprintf(buf + start + 1, 3, "%02x", r);
      snprintf(buf + start + 3, 3, "%02x", g);
      snprintf(buf + start + 5, 3, "%02x", b);
    }
  }

  return true; 
}

// -- Phase four: collapse hex values --

bool p4(char* buf, unsigned int bufsz) {
  for (unsigned int i = 7; i < bufsz; i++) {
    if (buf[i-7] == '#') {
      if (hex(buf[i-6]) && hex(buf[i-5]) && hex(buf[i-4]) && hex(buf[i-3]) && hex(buf[i-2]) && hex(buf[i-1]) && !hex(buf[i])) {
        if (buf[i-6] == buf[i-5] && buf[i-4] == buf[i-3] && buf[i-2] && buf[i-1]) {
          buf[i-5] = buf[i-4];
          buf[i-4] = buf[i-2];
          
          buf[i-3] = '\0';
          buf[i-2] = '\0';
          buf[i-1] = '\0';
        }
      }
    }
  }

  return true;
}

// -- Phase five: collapse zero values --

bool p5(char* buf, unsigned int bufsz) {
  for (unsigned int i = 2; i < bufsz; i++) {
    if (buf[i-2] == ':' && buf[i-1] == '0' && buf[i] == '.') s(buf, i-1);
  }
  return true;
}

// -- Phase six: collapse unneeded semicolons --

bool p6(char* buf, unsigned int bufsz) {
  for (unsigned int i = 1; i < bufsz; i++) {
    if (buf[i-1] == ';' && buf[i] == ';') s(buf, i);
    if (buf[i-1] == ';' && buf[i] == '}') s(buf, i-1);
  }

  return true;
}

// -- Phase seven: collapse empty declarations --

bool p7(char* buf, unsigned int bufsz) {
  for (unsigned int i = 1; i < bufsz; i++) {
    if (buf[i-1] == '{' && buf[i] == '}') {
      s(buf, i);
      unsigned int j;
      for (j = i; j != 0; j--) {
        if (buf[j] == '}') break;
      }

      if (j != 0) memset(buf + j + 1, 0, i - j);
    }
  }

  return true;
}

// -- Runner --

int main(int argc, char* argv[]) {
  if (argc < 2) error(1, 0, "Usage: turbougly <file>");

  // Read and measure the file
  FILE* fd;
  fd = fopen(argv[1], "r");
  if (fd == NULL) error(1, 0, "Couldn't open file");
  fseek(fd, 0L, SEEK_END);
  unsigned int bufsz = (unsigned int)ftell(fd) + 1;
  fseek(fd, 0L, SEEK_SET);

  // Allocate, initialize and fill a large enough buffer (plus padding)
  char* buf = (char*)calloc(bufsz, 1);
  fread(buf, bufsz, 1, fd);

  // Run the phases in succession
  if (!p0(buf, bufsz)) error(1, 0, "Error running phase 0"); shuffle(buf, bufsz);
  if (!p1(buf, bufsz)) error(1, 0, "Error running phase 1"); shuffle(buf, bufsz);
  if (!p2(buf, bufsz)) error(1, 0, "Error running phase 2"); shuffle(buf, bufsz);
  if (!p3(buf, bufsz)) error(1, 0, "Error running phase 3"); shuffle(buf, bufsz);
  if (!p4(buf, bufsz)) error(1, 0, "Error running phase 4"); shuffle(buf, bufsz);
  if (!p5(buf, bufsz)) error(1, 0, "Error running phase 5"); shuffle(buf, bufsz);
  if (!p6(buf, bufsz)) error(1, 0, "Error running phase 6"); shuffle(buf, bufsz);
  if (!p7(buf, bufsz)) error(1, 0, "Error running phase 7"); shuffle(buf, bufsz);

  // Print the result
  fputs(buf, stdout);

  // Clean up
  free(buf);
  fclose(fd);
}
