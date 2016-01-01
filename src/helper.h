#ifndef _HELPER_H_
#define _HELPER_H_

#include <cstdio>
#include <cstdint>
#ifdef TOY8086_UNIX
#  include <unistd.h>
#  include <termios.h>
#endif
#ifdef TOY8086_WIN32
#  include <conio.h>
#endif

using byte = uint8_t;
using word = uint16_t;
using dword = uint32_t;

template<typename T>
inline word &to_word(T *p) {
  return *reinterpret_cast<word *>(p);
}

template<typename T>
inline byte &to_byte(T *p) {
  return *reinterpret_cast<byte *>(p);
}

template<typename T>
inline word &to_word(T &p) {
  return reinterpret_cast<word &>(p);
}

template<typename T>
inline byte &to_byte(T &p) {
  return reinterpret_cast<byte &>(p);
}

template<typename T>
inline void *to_ptr(T &p) {
  return &p;
}


inline int getonechar(bool echo) {
#ifdef TOY8086_UNIX
  struct termios oldt, newt;

  /*tcgetattr gets the parameters of the current terminal
  STDIN_FILENO will tell tcgetattr that it should write the settings
  of stdin to oldt*/
  tcgetattr(STDIN_FILENO, &oldt);
  /*now the settings will be copied*/
  newt = oldt;

  /*ICANON normally takes care that one line at a time will be processed
  that means it will return if it sees a "\n" or an EOF or an EOL*/
  newt.c_lflag &= ~(ICANON);
  if (!echo) {
    newt.c_lflag &= ~(ECHO);
  }

  /*Those new settings will be set to STDIN
  TCSANOW tells tcsetattr to change attributes immediately. */
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  /*This is your part:
  I choose 'e' to end input. Notice that EOF is also turned off
  in the non-canonical mode*/
  int c = getchar();
  /*restore the old settings*/
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return c;
#endif
#ifdef TOY8086_MSVC
  int c = _getch();  // getch() catch input unbuffered but with no echo.
  if (echo) {
    printf("%c", c);
  }
  return c;
#endif
}

#endif
