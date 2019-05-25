
#ifndef GTEST_INCLUDE_GTEST_GTEST_TEST_PART_H_
#define GTEST_INCLUDE_GTEST_GTEST_TEST_PART_H_

#include <iosfwd>
#include <vector>
#include "gtest/internal/gtest-internal.h"
#include "gtest/internal/gtest-string.h"

namespace testing {

class GTEST_API_ TestPartResult {
 public:
  enum Type {
    kSuccess,          // Succeeded.
    kNonFatalFailure,  // Failed but the test can continue.
    kFatalFailure      // Failed and the test should be terminated.
  };

  TestPartResult(Type a_type,
                 const char* a_file_name,
                 int a_line_number,
                 const char* a_message)
      : type_(a_type),
        file_name_(a_file_name),
        line_number_(a_line_number),
        summary_(ExtractSummary(a_message)),
        message_(a_message) {
  }

  Type type() const { return type_; }

  const char* file_name() const { return file_name_.c_str(); }

  int line_number() const { return line_number_; }

  const char* summary() const { return summary_.c_str(); }

  const char* message() const { return message_.c_str(); }

  bool passed() const { return type_ == kSuccess; }

  bool failed() const { return type_ != kSuccess; }

  bool nonfatally_failed() const { return type_ == kNonFatalFailure; }

  bool fatally_failed() const { return type_ == kFatalFailure; }
 private:
  Type type_;

  static internal::String ExtractSummary(const char* message);

  internal::String file_name_;
  int line_number_;
  internal::String summary_;  // The test failure summary.
  internal::String message_;  // The test failure message.
};

std::ostream& operator<<(std::ostream& os, const TestPartResult& result);

class GTEST_API_ TestPartResultArray {
 public:
  TestPartResultArray() {}

  void Append(const TestPartResult& result);

  const TestPartResult& GetTestPartResult(int index) const;

  int size() const;

 private:
  std::vector<TestPartResult> array_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestPartResultArray);
};

class TestPartResultReporterInterface {
 public:
  virtual ~TestPartResultReporterInterface() {}

  virtual void ReportTestPartResult(const TestPartResult& result) = 0;
};

namespace internal {

class GTEST_API_ HasNewFatalFailureHelper
    : public TestPartResultReporterInterface {
 public:
  HasNewFatalFailureHelper();
  virtual ~HasNewFatalFailureHelper();
  virtual void ReportTestPartResult(const TestPartResult& result);
  bool has_new_fatal_failure() const { return has_new_fatal_failure_; }
 private:
  bool has_new_fatal_failure_;
  TestPartResultReporterInterface* original_reporter_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(HasNewFatalFailureHelper);
};

}  // namespace internal

}  // namespace testing

#endif  // GTEST_INCLUDE_GTEST_GTEST_TEST_PART_H_
