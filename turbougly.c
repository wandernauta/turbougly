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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <error.h>
#include <ctypes.h>
#include <string.h>
#include <strings.h> // NOTE: bzero is deprecated

// -- Utility function prototypes --

void s(char*, unsigned int);
bool space(char);
bool hex(char);
char next(char*, unsigned int);
char nnext(char*, unsigned int);
char prev(char*, unsigned int);
char nprev(char*, unsigned int);

// -- Utility functions --

void s(char* buf, unsigned int i) { bzero(buf + i, 1); }
bool space(char c) { return (c == ' ' || c == '\t' || c == '\n'); }
bool hex(char c) { return isxdigit(c); }
char next(char* buf, unsigned int i) { while (buf[i] == '\0') i++; return buf[i]; }
char nnext(char* buf, unsigned int i) { while (buf[i]=='\0'||space(buf[i])) i++; return buf[i];}
char prev(char* buf, unsigned int i) { while (buf[i] == '\0') i--; return buf[i]; }
char nprev(char* buf, unsigned int i) { while (buf[i]=='\0'||space(buf[i])) i--; return buf[i];}

// -- Phase function prototypes --

bool p0(char*, unsigned int);
bool p1(char*, unsigned int);
bool p2(char*, unsigned int);
bool p3(char*, unsigned int);
bool p4(char*, unsigned int);
bool p5(char*, unsigned int);
bool p6(char*, unsigned int);

// -- Phase zero: obliterate tabs -- 

bool p0(char* buf, unsigned int bufsz) {
  for (unsigned int i = 0; i < bufsz; ++i) {
    if (buf[i] == '\t') s(buf, i);
  }

  return true;
}

// -- Phase one: strip comments --

bool p1(char* buf, unsigned int bufsz) {
  bool in_comment = false;

  for (unsigned int i = 0; i < bufsz; ++i) {
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
  for (unsigned int i = 0; i < bufsz; i++) {
    if (buf[i] == '\n') s(buf, i);
    if (buf[i] == ' ' && next(buf, i+1) == ' ') s(buf, i);
    if (buf[i] == ' ' && nnext(buf, i) == '{') s(buf, i);
    if (buf[i] == ' ' && nprev(buf, i) == '{') s(buf, i);
    if (buf[i] == ' ' && nprev(buf, i) == ':') s(buf, i);
    if (buf[i] == ' ' && nnext(buf, i) == ':') s(buf, i); // TODO Pseudo?
    if (buf[i] == ' ' && nprev(buf, i) == ';') s(buf, i);
    if (buf[i] == ' ' && nprev(buf, i) == ',') s(buf, i);
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
      bzero(buf + start, i - start);

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
  for (unsigned int i = 1; i < bufsz; i++) {
    if (nprev(buf, i-1) == ':' && buf[i] == '0' && buf[i+1] == '.') s(buf, i);
  }
  return true;
}

// -- Phase six: collapse unneeded semicolons --

bool p6(char* buf, unsigned int bufsz) {
  for (unsigned int i = 0; i < bufsz; i++) {
    if (buf[i] == ';' && nnext(buf, i+1) == ';') s(buf, i);
    if (buf[i] == ';' && nnext(buf, i+1) == '}') s(buf, i);
  }

  return true;
}

// -- Runner --

int main(int argc, char* argv[]) {
  if (argc < 2) error(1, 0, "Usage: %s <file>", argv[0]);

  // Read and measure the file
  FILE* fd;
  fd = fopen(argv[1], "r");
  if (fd == NULL) error(1, 0, "Couldn't open file");
  fseek(fd, 0L, SEEK_END);
  unsigned int bufsz = (unsigned int)ftell(fd);
  fseek(fd, 0L, SEEK_SET);

  // Allocate, initialize and fill a large enough buffer
  char* buf = (char*)calloc(bufsz, 1);
  fread(buf, bufsz, 1, fd);

  // Run the phases in succession
  if (!p0(buf, bufsz)) error(1, 0, "Error running phase 0");
  if (!p1(buf, bufsz)) error(1, 0, "Error running phase 1");
  if (!p2(buf, bufsz)) error(1, 0, "Error running phase 2");
  if (!p3(buf, bufsz)) error(1, 0, "Error running phase 3");
  if (!p4(buf, bufsz)) error(1, 0, "Error running phase 4");
  if (!p5(buf, bufsz)) error(1, 0, "Error running phase 5");
  if (!p6(buf, bufsz)) error(1, 0, "Error running phase 6");

  // Print the result
  for (unsigned int i = 0; i < bufsz; i++) {
    if (buf[i] != '\0') putc(buf[i], stdout);
  }

  // Clean up
  free(buf);
  fclose(fd);
}
