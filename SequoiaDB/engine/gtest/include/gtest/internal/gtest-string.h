
#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_STRING_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_STRING_H_

#ifdef __BORLANDC__
# include <mem.h>
#endif

#include <string.h>
#include "gtest/internal/gtest-port.h"

#include <string>

namespace testing {
namespace internal {

class GTEST_API_ String {
 public:

  static String ShowCStringQuoted(const char* c_str);

  static const char* CloneCString(const char* c_str);

#if GTEST_OS_WINDOWS_MOBILE

  static LPCWSTR AnsiToUtf16(const char* c_str);

  static const char* Utf16ToAnsi(LPCWSTR utf16_str);
#endif

  static bool CStringEquals(const char* lhs, const char* rhs);

  static String ShowWideCString(const wchar_t* wide_c_str);

  static String ShowWideCStringQuoted(const wchar_t* wide_c_str);

  static bool WideCStringEquals(const wchar_t* lhs, const wchar_t* rhs);

  static bool CaseInsensitiveCStringEquals(const char* lhs,
                                           const char* rhs);

  static bool CaseInsensitiveWideCStringEquals(const wchar_t* lhs,
                                               const wchar_t* rhs);

  static String Format(const char* format, ...);


  String() : c_str_(NULL), length_(0) {}

  String(const char* a_c_str) {  // NOLINT
    if (a_c_str == NULL) {
      c_str_ = NULL;
      length_ = 0;
    } else {
      ConstructNonNull(a_c_str, strlen(a_c_str));
    }
  }

  String(const char* buffer, size_t a_length) {
    ConstructNonNull(buffer, a_length);
  }

  String(const String& str) : c_str_(NULL), length_(0) { *this = str; }

  ~String() { delete[] c_str_; }

  String(const ::std::string& str) {
    ConstructNonNull(str.c_str(), str.length());
  }

  operator ::std::string() const { return ::std::string(c_str(), length()); }

#if GTEST_HAS_GLOBAL_STRING
  String(const ::string& str) {
    ConstructNonNull(str.c_str(), str.length());
  }

  operator ::string() const { return ::string(c_str(), length()); }
#endif  // GTEST_HAS_GLOBAL_STRING

  bool empty() const { return (c_str() != NULL) && (length() == 0); }

  int Compare(const String& rhs) const;

  bool operator==(const char* a_c_str) const { return Compare(a_c_str) == 0; }

  bool operator<(const String& rhs) const { return Compare(rhs) < 0; }

  bool operator!=(const char* a_c_str) const { return !(*this == a_c_str); }

  bool EndsWith(const char* suffix) const;

  bool EndsWithCaseInsensitive(const char* suffix) const;

  size_t length() const { return length_; }

  const char* c_str() const { return c_str_; }

  const String& operator=(const char* a_c_str) {
    return *this = String(a_c_str);
  }

  const String& operator=(const String& rhs) {
    if (this != &rhs) {
      delete[] c_str_;
      if (rhs.c_str() == NULL) {
        c_str_ = NULL;
        length_ = 0;
      } else {
        ConstructNonNull(rhs.c_str(), rhs.length());
      }
    }

    return *this;
  }

 private:
  void ConstructNonNull(const char* buffer, size_t a_length) {
    char* const str = new char[a_length + 1];
    memcpy(str, buffer, a_length);
    str[a_length] = '\0';
    c_str_ = str;
    length_ = a_length;
  }

  const char* c_str_;
  size_t length_;
};  // class String

inline ::std::ostream& operator<<(::std::ostream& os, const String& str) {
  if (str.c_str() == NULL) {
    os << "(null)";
  } else {
    const char* const c_str = str.c_str();
    for (size_t i = 0; i != str.length(); i++) {
      if (c_str[i] == '\0') {
        os << "\\0";
      } else {
        os << c_str[i];
      }
    }
  }
  return os;
}

GTEST_API_ String StringStreamToString(::std::stringstream* stream);


template <typename T>
String StreamableToString(const T& streamable);

}  // namespace internal
}  // namespace testing

#endif  // GTEST_INCLUDE_GTEST_INTERNAL_GTEST_STRING_H_
