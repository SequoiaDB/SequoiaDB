

#ifndef GTEST_INCLUDE_GTEST_GTEST_PRINTERS_H_
#define GTEST_INCLUDE_GTEST_GTEST_PRINTERS_H_

#include <ostream>  // NOLINT
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "gtest/internal/gtest-port.h"
#include "gtest/internal/gtest-internal.h"

namespace testing {

namespace internal2 {

GTEST_API_ void PrintBytesInObjectTo(const unsigned char* obj_bytes,
                                     size_t count,
                                     ::std::ostream* os);

enum TypeKind {
  kProtobuf,              // a protobuf type
  kConvertibleToInteger,  // a type implicitly convertible to BiggestInt
  kOtherType              // anything else
};

template <typename T, TypeKind kTypeKind>
class TypeWithoutFormatter {
 public:
  static void PrintValue(const T& value, ::std::ostream* os) {
    PrintBytesInObjectTo(reinterpret_cast<const unsigned char*>(&value),
                         sizeof(value), os);
  }
};

const size_t kProtobufOneLinerMaxLength = 50;

template <typename T>
class TypeWithoutFormatter<T, kProtobuf> {
 public:
  static void PrintValue(const T& value, ::std::ostream* os) {
    const ::testing::internal::string short_str = value.ShortDebugString();
    const ::testing::internal::string pretty_str =
        short_str.length() <= kProtobufOneLinerMaxLength ?
        short_str : ("\n" + value.DebugString());
    *os << ("<" + pretty_str + ">");
  }
};

template <typename T>
class TypeWithoutFormatter<T, kConvertibleToInteger> {
 public:
  static void PrintValue(const T& value, ::std::ostream* os) {
    const internal::BiggestInt kBigInt = value;
    *os << kBigInt;
  }
};

template <typename Char, typename CharTraits, typename T>
::std::basic_ostream<Char, CharTraits>& operator<<(
    ::std::basic_ostream<Char, CharTraits>& os, const T& x) {
  TypeWithoutFormatter<T,
      (internal::IsAProtocolMessage<T>::value ? kProtobuf :
       internal::ImplicitlyConvertible<const T&, internal::BiggestInt>::value ?
       kConvertibleToInteger : kOtherType)>::PrintValue(x, &os);
  return os;
}

}  // namespace internal2
}  // namespace testing

namespace testing_internal {

template <typename T>
void DefaultPrintNonContainerTo(const T& value, ::std::ostream* os) {
  using namespace ::testing::internal2;  // NOLINT

  *os << value;
}

}  // namespace testing_internal

namespace testing {
namespace internal {

template <typename T>
class UniversalPrinter;

template <typename T>
void UniversalPrint(const T& value, ::std::ostream* os);

template <typename C>
void DefaultPrintTo(IsContainer /* dummy */,
                    false_type /* is not a pointer */,
                    const C& container, ::std::ostream* os) {
  const size_t kMaxCount = 32;  // The maximum number of elements to print.
  *os << '{';
  size_t count = 0;
  for (typename C::const_iterator it = container.begin();
       it != container.end(); ++it, ++count) {
    if (count > 0) {
      *os << ',';
      if (count == kMaxCount) {  // Enough has been printed.
        *os << " ...";
        break;
      }
    }
    *os << ' ';
    internal::UniversalPrint(*it, os);
  }

  if (count > 0) {
    *os << ' ';
  }
  *os << '}';
}

template <typename T>
void DefaultPrintTo(IsNotContainer /* dummy */,
                    true_type /* is a pointer */,
                    T* p, ::std::ostream* os) {
  if (p == NULL) {
    *os << "NULL";
  } else {
    if (IsTrue(ImplicitlyConvertible<T*, const void*>::value)) {
      *os << p;
    } else {
      *os << reinterpret_cast<const void*>(
          reinterpret_cast<internal::UInt64>(p));
    }
  }
}

template <typename T>
void DefaultPrintTo(IsNotContainer /* dummy */,
                    false_type /* is not a pointer */,
                    const T& value, ::std::ostream* os) {
  ::testing_internal::DefaultPrintNonContainerTo(value, os);
}

template <typename T>
void PrintTo(const T& value, ::std::ostream* os) {
  DefaultPrintTo(IsContainerTest<T>(0), is_pointer<T>(), value, os);
}


GTEST_API_ void PrintTo(unsigned char c, ::std::ostream* os);
GTEST_API_ void PrintTo(signed char c, ::std::ostream* os);
inline void PrintTo(char c, ::std::ostream* os) {
  PrintTo(static_cast<unsigned char>(c), os);
}

inline void PrintTo(bool x, ::std::ostream* os) {
  *os << (x ? "true" : "false");
}

GTEST_API_ void PrintTo(wchar_t wc, ::std::ostream* os);

GTEST_API_ void PrintTo(const char* s, ::std::ostream* os);
inline void PrintTo(char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const char*>(s), os);
}

inline void PrintTo(const signed char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}
inline void PrintTo(signed char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}
inline void PrintTo(const unsigned char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}
inline void PrintTo(unsigned char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}

#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
GTEST_API_ void PrintTo(const wchar_t* s, ::std::ostream* os);
inline void PrintTo(wchar_t* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const wchar_t*>(s), os);
}
#endif


template <typename T>
void PrintRawArrayTo(const T a[], size_t count, ::std::ostream* os) {
  UniversalPrint(a[0], os);
  for (size_t i = 1; i != count; i++) {
    *os << ", ";
    UniversalPrint(a[i], os);
  }
}

#if GTEST_HAS_GLOBAL_STRING
GTEST_API_ void PrintStringTo(const ::string&s, ::std::ostream* os);
inline void PrintTo(const ::string& s, ::std::ostream* os) {
  PrintStringTo(s, os);
}
#endif  // GTEST_HAS_GLOBAL_STRING

GTEST_API_ void PrintStringTo(const ::std::string&s, ::std::ostream* os);
inline void PrintTo(const ::std::string& s, ::std::ostream* os) {
  PrintStringTo(s, os);
}

#if GTEST_HAS_GLOBAL_WSTRING
GTEST_API_ void PrintWideStringTo(const ::wstring&s, ::std::ostream* os);
inline void PrintTo(const ::wstring& s, ::std::ostream* os) {
  PrintWideStringTo(s, os);
}
#endif  // GTEST_HAS_GLOBAL_WSTRING

#if GTEST_HAS_STD_WSTRING
GTEST_API_ void PrintWideStringTo(const ::std::wstring&s, ::std::ostream* os);
inline void PrintTo(const ::std::wstring& s, ::std::ostream* os) {
  PrintWideStringTo(s, os);
}
#endif  // GTEST_HAS_STD_WSTRING

#if GTEST_HAS_TR1_TUPLE

template <typename T>
void PrintTupleTo(const T& t, ::std::ostream* os);


inline void PrintTo(const ::std::tr1::tuple<>& t, ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1>
void PrintTo(const ::std::tr1::tuple<T1>& t, ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2>
void PrintTo(const ::std::tr1::tuple<T1, T2>& t, ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3>
void PrintTo(const ::std::tr1::tuple<T1, T2, T3>& t, ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3, typename T4>
void PrintTo(const ::std::tr1::tuple<T1, T2, T3, T4>& t, ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
void PrintTo(const ::std::tr1::tuple<T1, T2, T3, T4, T5>& t,
             ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
void PrintTo(const ::std::tr1::tuple<T1, T2, T3, T4, T5, T6>& t,
             ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
void PrintTo(const ::std::tr1::tuple<T1, T2, T3, T4, T5, T6, T7>& t,
             ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
void PrintTo(const ::std::tr1::tuple<T1, T2, T3, T4, T5, T6, T7, T8>& t,
             ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9>
void PrintTo(const ::std::tr1::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9>& t,
             ::std::ostream* os) {
  PrintTupleTo(t, os);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10>
void PrintTo(
    const ::std::tr1::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& t,
    ::std::ostream* os) {
  PrintTupleTo(t, os);
}
#endif  // GTEST_HAS_TR1_TUPLE

template <typename T1, typename T2>
void PrintTo(const ::std::pair<T1, T2>& value, ::std::ostream* os) {
  *os << '(';
  UniversalPrinter<T1>::Print(value.first, os);
  *os << ", ";
  UniversalPrinter<T2>::Print(value.second, os);
  *os << ')';
}

template <typename T>
class UniversalPrinter {
 public:
#ifdef _MSC_VER
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4180)  // Temporarily disables warning 4180.
#endif  // _MSC_VER

  static void Print(const T& value, ::std::ostream* os) {
    PrintTo(value, os);
  }

#ifdef _MSC_VER
# pragma warning(pop)           // Restores the warning state.
#endif  // _MSC_VER
};

template <typename T>
void UniversalPrintArray(const T* begin, size_t len, ::std::ostream* os) {
  if (len == 0) {
    *os << "{}";
  } else {
    *os << "{ ";
    const size_t kThreshold = 18;
    const size_t kChunkSize = 8;
    if (len <= kThreshold) {
      PrintRawArrayTo(begin, len, os);
    } else {
      PrintRawArrayTo(begin, kChunkSize, os);
      *os << ", ..., ";
      PrintRawArrayTo(begin + len - kChunkSize, kChunkSize, os);
    }
    *os << " }";
  }
}
GTEST_API_ void UniversalPrintArray(const char* begin,
                                    size_t len,
                                    ::std::ostream* os);

template <typename T, size_t N>
class UniversalPrinter<T[N]> {
 public:
  static void Print(const T (&a)[N], ::std::ostream* os) {
    UniversalPrintArray(a, N, os);
  }
};

template <typename T>
class UniversalPrinter<T&> {
 public:
#ifdef _MSC_VER
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4180)  // Temporarily disables warning 4180.
#endif  // _MSC_VER

  static void Print(const T& value, ::std::ostream* os) {
    *os << "@" << reinterpret_cast<const void*>(&value) << " ";

    UniversalPrint(value, os);
  }

#ifdef _MSC_VER
# pragma warning(pop)           // Restores the warning state.
#endif  // _MSC_VER
};

template <typename T>
void UniversalTersePrint(const T& value, ::std::ostream* os) {
  UniversalPrint(value, os);
}
inline void UniversalTersePrint(const char* str, ::std::ostream* os) {
  if (str == NULL) {
    *os << "NULL";
  } else {
    UniversalPrint(string(str), os);
  }
}
inline void UniversalTersePrint(char* str, ::std::ostream* os) {
  UniversalTersePrint(static_cast<const char*>(str), os);
}

template <typename T>
void UniversalPrint(const T& value, ::std::ostream* os) {
  UniversalPrinter<T>::Print(value, os);
}

#if GTEST_HAS_TR1_TUPLE
typedef ::std::vector<string> Strings;


template <size_t N>
struct TuplePrefixPrinter {
  template <typename Tuple>
  static void PrintPrefixTo(const Tuple& t, ::std::ostream* os) {
    TuplePrefixPrinter<N - 1>::PrintPrefixTo(t, os);
    *os << ", ";
    UniversalPrinter<typename ::std::tr1::tuple_element<N - 1, Tuple>::type>
        ::Print(::std::tr1::get<N - 1>(t), os);
  }

  template <typename Tuple>
  static void TersePrintPrefixToStrings(const Tuple& t, Strings* strings) {
    TuplePrefixPrinter<N - 1>::TersePrintPrefixToStrings(t, strings);
    ::std::stringstream ss;
    UniversalTersePrint(::std::tr1::get<N - 1>(t), &ss);
    strings->push_back(ss.str());
  }
};

template <>
struct TuplePrefixPrinter<0> {
  template <typename Tuple>
  static void PrintPrefixTo(const Tuple&, ::std::ostream*) {}

  template <typename Tuple>
  static void TersePrintPrefixToStrings(const Tuple&, Strings*) {}
};
template <>
struct TuplePrefixPrinter<1> {
  template <typename Tuple>
  static void PrintPrefixTo(const Tuple& t, ::std::ostream* os) {
    UniversalPrinter<typename ::std::tr1::tuple_element<0, Tuple>::type>::
        Print(::std::tr1::get<0>(t), os);
  }

  template <typename Tuple>
  static void TersePrintPrefixToStrings(const Tuple& t, Strings* strings) {
    ::std::stringstream ss;
    UniversalTersePrint(::std::tr1::get<0>(t), &ss);
    strings->push_back(ss.str());
  }
};

template <typename T>
void PrintTupleTo(const T& t, ::std::ostream* os) {
  *os << "(";
  TuplePrefixPrinter< ::std::tr1::tuple_size<T>::value>::
      PrintPrefixTo(t, os);
  *os << ")";
}

template <typename Tuple>
Strings UniversalTersePrintTupleFieldsToStrings(const Tuple& value) {
  Strings result;
  TuplePrefixPrinter< ::std::tr1::tuple_size<Tuple>::value>::
      TersePrintPrefixToStrings(value, &result);
  return result;
}
#endif  // GTEST_HAS_TR1_TUPLE

}  // namespace internal

template <typename T>
::std::string PrintToString(const T& value) {
  ::std::stringstream ss;
  internal::UniversalTersePrint(value, &ss);
  return ss.str();
}

}  // namespace testing

#endif  // GTEST_INCLUDE_GTEST_GTEST_PRINTERS_H_
