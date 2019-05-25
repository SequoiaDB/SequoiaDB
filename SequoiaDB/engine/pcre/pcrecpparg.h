
#ifndef _PCRECPPARG_H
#define _PCRECPPARG_H

#include <stdlib.h>    // for NULL
#include <string>

#include "pcre.h"

namespace pcrecpp {

class StringPiece;


template <class T>
class _RE_MatchObject {
 public:
  static inline bool Parse(const char* str, int n, void* dest) {
    if (dest == NULL) return true;
    T* object = reinterpret_cast<T*>(dest);
    return object->ParseFrom(str, n);
  }
};

class PCRECPP_EXP_DEFN Arg {
 public:
  Arg();

  Arg(void*);

  typedef bool (*Parser)(const char* str, int n, void* dest);

#define PCRE_MAKE_PARSER(type,name)                             \
  Arg(type* p) : arg_(p), parser_(name) { }                     \
  Arg(type* p, Parser parser) : arg_(p), parser_(parser) { }


  PCRE_MAKE_PARSER(char,               parse_char);
  PCRE_MAKE_PARSER(unsigned char,      parse_uchar);
  PCRE_MAKE_PARSER(short,              parse_short);
  PCRE_MAKE_PARSER(unsigned short,     parse_ushort);
  PCRE_MAKE_PARSER(int,                parse_int);
  PCRE_MAKE_PARSER(unsigned int,       parse_uint);
  PCRE_MAKE_PARSER(long,               parse_long);
  PCRE_MAKE_PARSER(unsigned long,      parse_ulong);
#if 1
  PCRE_MAKE_PARSER(long long,          parse_longlong);
#endif
#if 1
  PCRE_MAKE_PARSER(unsigned long long, parse_ulonglong);
#endif
  PCRE_MAKE_PARSER(float,              parse_float);
  PCRE_MAKE_PARSER(double,             parse_double);
  PCRE_MAKE_PARSER(std::string,        parse_string);
  PCRE_MAKE_PARSER(StringPiece,        parse_stringpiece);

#undef PCRE_MAKE_PARSER

  template <class T> Arg(T*, Parser parser);
  template <class T> Arg(T* p)
    : arg_(p), parser_(_RE_MatchObject<T>::Parse) {
  }

  bool Parse(const char* str, int n) const;

 private:
  void*         arg_;
  Parser        parser_;

  static bool parse_null          (const char* str, int n, void* dest);
  static bool parse_char          (const char* str, int n, void* dest);
  static bool parse_uchar         (const char* str, int n, void* dest);
  static bool parse_float         (const char* str, int n, void* dest);
  static bool parse_double        (const char* str, int n, void* dest);
  static bool parse_string        (const char* str, int n, void* dest);
  static bool parse_stringpiece   (const char* str, int n, void* dest);

#define PCRE_DECLARE_INTEGER_PARSER(name)                                   \
 private:                                                                   \
  static bool parse_ ## name(const char* str, int n, void* dest);           \
  static bool parse_ ## name ## _radix(                                     \
    const char* str, int n, void* dest, int radix);                         \
 public:                                                                    \
  static bool parse_ ## name ## _hex(const char* str, int n, void* dest);   \
  static bool parse_ ## name ## _octal(const char* str, int n, void* dest); \
  static bool parse_ ## name ## _cradix(const char* str, int n, void* dest)

  PCRE_DECLARE_INTEGER_PARSER(short);
  PCRE_DECLARE_INTEGER_PARSER(ushort);
  PCRE_DECLARE_INTEGER_PARSER(int);
  PCRE_DECLARE_INTEGER_PARSER(uint);
  PCRE_DECLARE_INTEGER_PARSER(long);
  PCRE_DECLARE_INTEGER_PARSER(ulong);
  PCRE_DECLARE_INTEGER_PARSER(longlong);
  PCRE_DECLARE_INTEGER_PARSER(ulonglong);

#undef PCRE_DECLARE_INTEGER_PARSER
};

inline Arg::Arg() : arg_(NULL), parser_(parse_null) { }
inline Arg::Arg(void* p) : arg_(p), parser_(parse_null) { }

inline bool Arg::Parse(const char* str, int n) const {
  return (*parser_)(str, n, arg_);
}

#define MAKE_INTEGER_PARSER(type, name) \
  inline Arg Hex(type* ptr) { \
    return Arg(ptr, Arg::parse_ ## name ## _hex); } \
  inline Arg Octal(type* ptr) { \
    return Arg(ptr, Arg::parse_ ## name ## _octal); } \
  inline Arg CRadix(type* ptr) { \
    return Arg(ptr, Arg::parse_ ## name ## _cradix); }

MAKE_INTEGER_PARSER(short,              short)     /*                        */
MAKE_INTEGER_PARSER(unsigned short,     ushort)    /*                        */
MAKE_INTEGER_PARSER(int,                int)       /* Don't use semicolons   */
MAKE_INTEGER_PARSER(unsigned int,       uint)      /* after these statement  */
MAKE_INTEGER_PARSER(long,               long)      /* because they can cause */
MAKE_INTEGER_PARSER(unsigned long,      ulong)     /* compiler warnings if   */
#if 1                          /* the checking level is  */
MAKE_INTEGER_PARSER(long long,          longlong)  /* turned up high enough. */
#endif                                             /*                        */
#if 1                         /*                        */
MAKE_INTEGER_PARSER(unsigned long long, ulonglong) /*                        */
#endif

#undef PCRE_IS_SET
#undef PCRE_SET_OR_CLEAR
#undef MAKE_INTEGER_PARSER

}   // namespace pcrecpp


#endif /* _PCRECPPARG_H */
