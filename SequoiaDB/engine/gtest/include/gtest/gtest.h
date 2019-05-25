
#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
#define GTEST_INCLUDE_GTEST_GTEST_H_

#include <limits>
#include <vector>

#include "gtest/internal/gtest-internal.h"
#include "gtest/internal/gtest-string.h"
#include "gtest/gtest-death-test.h"
#include "gtest/gtest-message.h"
#include "gtest/gtest-param-test.h"
#include "gtest/gtest-printers.h"
#include "gtest/gtest_prod.h"
#include "gtest/gtest-test-part.h"
#include "gtest/gtest-typed-test.h"


namespace testing {


GTEST_DECLARE_bool_(also_run_disabled_tests);

GTEST_DECLARE_bool_(break_on_failure);

GTEST_DECLARE_bool_(catch_exceptions);

GTEST_DECLARE_string_(color);

GTEST_DECLARE_string_(filter);

GTEST_DECLARE_bool_(list_tests);

GTEST_DECLARE_string_(output);

GTEST_DECLARE_bool_(print_time);

GTEST_DECLARE_int32_(random_seed);

GTEST_DECLARE_int32_(repeat);

GTEST_DECLARE_bool_(show_internal_stack_frames);

GTEST_DECLARE_bool_(shuffle);

GTEST_DECLARE_int32_(stack_trace_depth);

GTEST_DECLARE_bool_(throw_on_failure);

GTEST_DECLARE_string_(stream_result_to);

const int kMaxStackTraceDepth = 100;

namespace internal {

class AssertHelper;
class DefaultGlobalTestPartResultReporter;
class ExecDeathTest;
class NoExecDeathTest;
class FinalSuccessChecker;
class GTestFlagSaver;
class TestResultAccessor;
class TestEventListenersAccessor;
class TestEventRepeater;
class WindowsDeathTest;
class UnitTestImpl* GetUnitTestImpl();
void ReportFailureInUnknownLocation(TestPartResult::Type result_type,
                                    const String& message);

template <typename T>
String StreamableToString(const T& streamable) {
  return (Message() << streamable).GetString();
}

}  // namespace internal

class Test;
class TestCase;
class TestInfo;
class UnitTest;

class GTEST_API_ AssertionResult {
 public:
  AssertionResult(const AssertionResult& other);
  explicit AssertionResult(bool success) : success_(success) {}

  operator bool() const { return success_; }  // NOLINT

  AssertionResult operator!() const;

  const char* message() const {
    return message_.get() != NULL ?  message_->c_str() : "";
  }
  const char* failure_message() const { return message(); }

  template <typename T> AssertionResult& operator<<(const T& value) {
    AppendMessage(Message() << value);
    return *this;
  }

  AssertionResult& operator<<(
      ::std::ostream& (*basic_manipulator)(::std::ostream& stream)) {
    AppendMessage(Message() << basic_manipulator);
    return *this;
  }

 private:
  void AppendMessage(const Message& a_message) {
    if (message_.get() == NULL)
      message_.reset(new ::std::string);
    message_->append(a_message.GetString().c_str());
  }

  bool success_;
  internal::scoped_ptr< ::std::string> message_;

  GTEST_DISALLOW_ASSIGN_(AssertionResult);
};

GTEST_API_ AssertionResult AssertionSuccess();

GTEST_API_ AssertionResult AssertionFailure();

GTEST_API_ AssertionResult AssertionFailure(const Message& msg);

class GTEST_API_ Test {
 public:
  friend class TestInfo;

  typedef internal::SetUpTestCaseFunc SetUpTestCaseFunc;
  typedef internal::TearDownTestCaseFunc TearDownTestCaseFunc;

  virtual ~Test();

  static void SetUpTestCase() {}

  static void TearDownTestCase() {}

  static bool HasFatalFailure();

  static bool HasNonfatalFailure();

  static bool HasFailure() { return HasFatalFailure() || HasNonfatalFailure(); }

  static void RecordProperty(const char* key, const char* value);
  static void RecordProperty(const char* key, int value);

 protected:
  Test();

  virtual void SetUp();

  virtual void TearDown();

 private:
  static bool HasSameFixtureClass();

  virtual void TestBody() = 0;

  void Run();

  void DeleteSelf_() { delete this; }

  const internal::GTestFlagSaver* const gtest_flag_saver_;

  struct Setup_should_be_spelled_SetUp {};
  virtual Setup_should_be_spelled_SetUp* Setup() { return NULL; }

  GTEST_DISALLOW_COPY_AND_ASSIGN_(Test);
};

typedef internal::TimeInMillis TimeInMillis;

class TestProperty {
 public:
  TestProperty(const char* a_key, const char* a_value) :
    key_(a_key), value_(a_value) {
  }

  const char* key() const {
    return key_.c_str();
  }

  const char* value() const {
    return value_.c_str();
  }

  void SetValue(const char* new_value) {
    value_ = new_value;
  }

 private:
  internal::String key_;
  internal::String value_;
};

class GTEST_API_ TestResult {
 public:
  TestResult();

  ~TestResult();

  int total_part_count() const;

  int test_property_count() const;

  bool Passed() const { return !Failed(); }

  bool Failed() const;

  bool HasFatalFailure() const;

  bool HasNonfatalFailure() const;

  TimeInMillis elapsed_time() const { return elapsed_time_; }

  const TestPartResult& GetTestPartResult(int i) const;

  const TestProperty& GetTestProperty(int i) const;

 private:
  friend class TestInfo;
  friend class UnitTest;
  friend class internal::DefaultGlobalTestPartResultReporter;
  friend class internal::ExecDeathTest;
  friend class internal::TestResultAccessor;
  friend class internal::UnitTestImpl;
  friend class internal::WindowsDeathTest;

  const std::vector<TestPartResult>& test_part_results() const {
    return test_part_results_;
  }

  const std::vector<TestProperty>& test_properties() const {
    return test_properties_;
  }

  void set_elapsed_time(TimeInMillis elapsed) { elapsed_time_ = elapsed; }

  void RecordProperty(const TestProperty& test_property);

  static bool ValidateTestProperty(const TestProperty& test_property);

  void AddTestPartResult(const TestPartResult& test_part_result);

  int death_test_count() const { return death_test_count_; }

  int increment_death_test_count() { return ++death_test_count_; }

  void ClearTestPartResults();

  void Clear();

  internal::Mutex test_properites_mutex_;

  std::vector<TestPartResult> test_part_results_;
  std::vector<TestProperty> test_properties_;
  int death_test_count_;
  TimeInMillis elapsed_time_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestResult);
};  // class TestResult

class GTEST_API_ TestInfo {
 public:
  ~TestInfo();

  const char* test_case_name() const { return test_case_name_.c_str(); }

  const char* name() const { return name_.c_str(); }

  const char* type_param() const {
    if (type_param_.get() != NULL)
      return type_param_->c_str();
    return NULL;
  }

  const char* value_param() const {
    if (value_param_.get() != NULL)
      return value_param_->c_str();
    return NULL;
  }

  bool should_run() const { return should_run_; }

  const TestResult* result() const { return &result_; }

 private:

#if GTEST_HAS_DEATH_TEST
  friend class internal::DefaultDeathTestFactory;
#endif  // GTEST_HAS_DEATH_TEST
  friend class Test;
  friend class TestCase;
  friend class internal::UnitTestImpl;
  friend TestInfo* internal::MakeAndRegisterTestInfo(
      const char* test_case_name, const char* name,
      const char* type_param,
      const char* value_param,
      internal::TypeId fixture_class_id,
      Test::SetUpTestCaseFunc set_up_tc,
      Test::TearDownTestCaseFunc tear_down_tc,
      internal::TestFactoryBase* factory);

  TestInfo(const char* test_case_name, const char* name,
           const char* a_type_param,
           const char* a_value_param,
           internal::TypeId fixture_class_id,
           internal::TestFactoryBase* factory);

  int increment_death_test_count() {
    return result_.increment_death_test_count();
  }

  void Run();

  static void ClearTestResult(TestInfo* test_info) {
    test_info->result_.Clear();
  }

  const std::string test_case_name_;     // Test case name
  const std::string name_;               // Test name
  const internal::scoped_ptr<const ::std::string> type_param_;
  const internal::scoped_ptr<const ::std::string> value_param_;
  const internal::TypeId fixture_class_id_;   // ID of the test fixture class
  bool should_run_;                 // True iff this test should run
  bool is_disabled_;                // True iff this test is disabled
  bool matches_filter_;             // True if this test matches the
  internal::TestFactoryBase* const factory_;  // The factory that creates

  TestResult result_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestInfo);
};

class GTEST_API_ TestCase {
 public:
  TestCase(const char* name, const char* a_type_param,
           Test::SetUpTestCaseFunc set_up_tc,
           Test::TearDownTestCaseFunc tear_down_tc);

  virtual ~TestCase();

  const char* name() const { return name_.c_str(); }

  const char* type_param() const {
    if (type_param_.get() != NULL)
      return type_param_->c_str();
    return NULL;
  }

  bool should_run() const { return should_run_; }

  int successful_test_count() const;

  int failed_test_count() const;

  int disabled_test_count() const;

  int test_to_run_count() const;

  int total_test_count() const;

  bool Passed() const { return !Failed(); }

  bool Failed() const { return failed_test_count() > 0; }

  TimeInMillis elapsed_time() const { return elapsed_time_; }

  const TestInfo* GetTestInfo(int i) const;

 private:
  friend class Test;
  friend class internal::UnitTestImpl;

  std::vector<TestInfo*>& test_info_list() { return test_info_list_; }

  const std::vector<TestInfo*>& test_info_list() const {
    return test_info_list_;
  }

  TestInfo* GetMutableTestInfo(int i);

  void set_should_run(bool should) { should_run_ = should; }

  void AddTestInfo(TestInfo * test_info);

  void ClearResult();

  static void ClearTestCaseResult(TestCase* test_case) {
    test_case->ClearResult();
  }

  void Run();

  void RunSetUpTestCase() { (*set_up_tc_)(); }

  void RunTearDownTestCase() { (*tear_down_tc_)(); }

  static bool TestPassed(const TestInfo* test_info) {
    return test_info->should_run() && test_info->result()->Passed();
  }

  static bool TestFailed(const TestInfo* test_info) {
    return test_info->should_run() && test_info->result()->Failed();
  }

  static bool TestDisabled(const TestInfo* test_info) {
    return test_info->is_disabled_;
  }

  static bool ShouldRunTest(const TestInfo* test_info) {
    return test_info->should_run();
  }

  void ShuffleTests(internal::Random* random);

  void UnshuffleTests();

  internal::String name_;
  const internal::scoped_ptr<const ::std::string> type_param_;
  std::vector<TestInfo*> test_info_list_;
  std::vector<int> test_indices_;
  Test::SetUpTestCaseFunc set_up_tc_;
  Test::TearDownTestCaseFunc tear_down_tc_;
  bool should_run_;
  TimeInMillis elapsed_time_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestCase);
};

class Environment {
 public:
  virtual ~Environment() {}

  virtual void SetUp() {}

  virtual void TearDown() {}
 private:
  struct Setup_should_be_spelled_SetUp {};
  virtual Setup_should_be_spelled_SetUp* Setup() { return NULL; }
};

class TestEventListener {
 public:
  virtual ~TestEventListener() {}

  virtual void OnTestProgramStart(const UnitTest& unit_test) = 0;

  virtual void OnTestIterationStart(const UnitTest& unit_test,
                                    int iteration) = 0;

  virtual void OnEnvironmentsSetUpStart(const UnitTest& unit_test) = 0;

  virtual void OnEnvironmentsSetUpEnd(const UnitTest& unit_test) = 0;

  virtual void OnTestCaseStart(const TestCase& test_case) = 0;

  virtual void OnTestStart(const TestInfo& test_info) = 0;

  virtual void OnTestPartResult(const TestPartResult& test_part_result) = 0;

  virtual void OnTestEnd(const TestInfo& test_info) = 0;

  virtual void OnTestCaseEnd(const TestCase& test_case) = 0;

  virtual void OnEnvironmentsTearDownStart(const UnitTest& unit_test) = 0;

  virtual void OnEnvironmentsTearDownEnd(const UnitTest& unit_test) = 0;

  virtual void OnTestIterationEnd(const UnitTest& unit_test,
                                  int iteration) = 0;

  virtual void OnTestProgramEnd(const UnitTest& unit_test) = 0;
};

class EmptyTestEventListener : public TestEventListener {
 public:
  virtual void OnTestProgramStart(const UnitTest& /*unit_test*/) {}
  virtual void OnTestIterationStart(const UnitTest& /*unit_test*/,
                                    int /*iteration*/) {}
  virtual void OnEnvironmentsSetUpStart(const UnitTest& /*unit_test*/) {}
  virtual void OnEnvironmentsSetUpEnd(const UnitTest& /*unit_test*/) {}
  virtual void OnTestCaseStart(const TestCase& /*test_case*/) {}
  virtual void OnTestStart(const TestInfo& /*test_info*/) {}
  virtual void OnTestPartResult(const TestPartResult& /*test_part_result*/) {}
  virtual void OnTestEnd(const TestInfo& /*test_info*/) {}
  virtual void OnTestCaseEnd(const TestCase& /*test_case*/) {}
  virtual void OnEnvironmentsTearDownStart(const UnitTest& /*unit_test*/) {}
  virtual void OnEnvironmentsTearDownEnd(const UnitTest& /*unit_test*/) {}
  virtual void OnTestIterationEnd(const UnitTest& /*unit_test*/,
                                  int /*iteration*/) {}
  virtual void OnTestProgramEnd(const UnitTest& /*unit_test*/) {}
};

class GTEST_API_ TestEventListeners {
 public:
  TestEventListeners();
  ~TestEventListeners();

  void Append(TestEventListener* listener);

  TestEventListener* Release(TestEventListener* listener);

  TestEventListener* default_result_printer() const {
    return default_result_printer_;
  }

  TestEventListener* default_xml_generator() const {
    return default_xml_generator_;
  }

 private:
  friend class TestCase;
  friend class TestInfo;
  friend class internal::DefaultGlobalTestPartResultReporter;
  friend class internal::NoExecDeathTest;
  friend class internal::TestEventListenersAccessor;
  friend class internal::UnitTestImpl;

  TestEventListener* repeater();

  void SetDefaultResultPrinter(TestEventListener* listener);

  void SetDefaultXmlGenerator(TestEventListener* listener);

  bool EventForwardingEnabled() const;
  void SuppressEventForwarding();

  internal::TestEventRepeater* repeater_;
  TestEventListener* default_result_printer_;
  TestEventListener* default_xml_generator_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestEventListeners);
};

class GTEST_API_ UnitTest {
 public:
  static UnitTest* GetInstance();

  int Run() GTEST_MUST_USE_RESULT_;

  const char* original_working_dir() const;

  const TestCase* current_test_case() const;

  const TestInfo* current_test_info() const;

  int random_seed() const;

#if GTEST_HAS_PARAM_TEST
  internal::ParameterizedTestCaseRegistry& parameterized_test_registry();
#endif  // GTEST_HAS_PARAM_TEST

  int successful_test_case_count() const;

  int failed_test_case_count() const;

  int total_test_case_count() const;

  int test_case_to_run_count() const;

  int successful_test_count() const;

  int failed_test_count() const;

  int disabled_test_count() const;

  int total_test_count() const;

  int test_to_run_count() const;

  TimeInMillis elapsed_time() const;

  bool Passed() const;

  bool Failed() const;

  const TestCase* GetTestCase(int i) const;

  TestEventListeners& listeners();

 private:
  Environment* AddEnvironment(Environment* env);

  void AddTestPartResult(TestPartResult::Type result_type,
                         const char* file_name,
                         int line_number,
                         const internal::String& message,
                         const internal::String& os_stack_trace);

  void RecordPropertyForCurrentTest(const char* key, const char* value);

  TestCase* GetMutableTestCase(int i);

  internal::UnitTestImpl* impl() { return impl_; }
  const internal::UnitTestImpl* impl() const { return impl_; }

  friend class Test;
  friend class internal::AssertHelper;
  friend class internal::ScopedTrace;
  friend Environment* AddGlobalTestEnvironment(Environment* env);
  friend internal::UnitTestImpl* internal::GetUnitTestImpl();
  friend void internal::ReportFailureInUnknownLocation(
      TestPartResult::Type result_type,
      const internal::String& message);

  UnitTest();

  virtual ~UnitTest();

  void PushGTestTrace(const internal::TraceInfo& trace);

  void PopGTestTrace();

  mutable internal::Mutex mutex_;

  internal::UnitTestImpl* impl_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(UnitTest);
};

inline Environment* AddGlobalTestEnvironment(Environment* env) {
  return UnitTest::GetInstance()->AddEnvironment(env);
}

GTEST_API_ void InitGoogleTest(int* argc, char** argv);

GTEST_API_ void InitGoogleTest(int* argc, wchar_t** argv);

namespace internal {

template <typename T1, typename T2>
String FormatForComparisonFailureMessage(const T1& value,
                                         const T2& /* other_operand */) {
  return ::testing::PrintToString(value);
}

template <typename T1, typename T2>
AssertionResult CmpHelperEQ(const char* expected_expression,
                            const char* actual_expression,
                            const T1& expected,
                            const T2& actual) {
#ifdef _MSC_VER
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4389)  // Temporarily disables warning on
#endif

  if (expected == actual) {
    return AssertionSuccess();
  }

#ifdef _MSC_VER
# pragma warning(pop)          // Restores the warning state.
#endif

  return EqFailure(expected_expression,
                   actual_expression,
                   FormatForComparisonFailureMessage(expected, actual),
                   FormatForComparisonFailureMessage(actual, expected),
                   false);
}

GTEST_API_ AssertionResult CmpHelperEQ(const char* expected_expression,
                                       const char* actual_expression,
                                       BiggestInt expected,
                                       BiggestInt actual);

template <bool lhs_is_null_literal>
class EqHelper {
 public:
  template <typename T1, typename T2>
  static AssertionResult Compare(const char* expected_expression,
                                 const char* actual_expression,
                                 const T1& expected,
                                 const T2& actual) {
    return CmpHelperEQ(expected_expression, actual_expression, expected,
                       actual);
  }

  static AssertionResult Compare(const char* expected_expression,
                                 const char* actual_expression,
                                 BiggestInt expected,
                                 BiggestInt actual) {
    return CmpHelperEQ(expected_expression, actual_expression, expected,
                       actual);
  }
};

template <>
class EqHelper<true> {
 public:
  template <typename T1, typename T2>
  static AssertionResult Compare(
      const char* expected_expression,
      const char* actual_expression,
      const T1& expected,
      const T2& actual,
      typename EnableIf<!is_pointer<T2>::value>::type* = 0) {
    return CmpHelperEQ(expected_expression, actual_expression, expected,
                       actual);
  }

  template <typename T>
  static AssertionResult Compare(
      const char* expected_expression,
      const char* actual_expression,
      Secret* /* expected (NULL) */,
      T* actual) {
    return CmpHelperEQ(expected_expression, actual_expression,
                       static_cast<T*>(NULL), actual);
  }
};

#define GTEST_IMPL_CMP_HELPER_(op_name, op)\
template <typename T1, typename T2>\
AssertionResult CmpHelper##op_name(const char* expr1, const char* expr2, \
                                   const T1& val1, const T2& val2) {\
  if (val1 op val2) {\
    return AssertionSuccess();\
  } else {\
    return AssertionFailure() \
        << "Expected: (" << expr1 << ") " #op " (" << expr2\
        << "), actual: " << FormatForComparisonFailureMessage(val1, val2)\
        << " vs " << FormatForComparisonFailureMessage(val2, val1);\
  }\
}\
GTEST_API_ AssertionResult CmpHelper##op_name(\
    const char* expr1, const char* expr2, BiggestInt val1, BiggestInt val2)


GTEST_IMPL_CMP_HELPER_(NE, !=);
GTEST_IMPL_CMP_HELPER_(LE, <=);
GTEST_IMPL_CMP_HELPER_(LT, < );
GTEST_IMPL_CMP_HELPER_(GE, >=);
GTEST_IMPL_CMP_HELPER_(GT, > );

#undef GTEST_IMPL_CMP_HELPER_

GTEST_API_ AssertionResult CmpHelperSTREQ(const char* expected_expression,
                                          const char* actual_expression,
                                          const char* expected,
                                          const char* actual);

GTEST_API_ AssertionResult CmpHelperSTRCASEEQ(const char* expected_expression,
                                              const char* actual_expression,
                                              const char* expected,
                                              const char* actual);

GTEST_API_ AssertionResult CmpHelperSTRNE(const char* s1_expression,
                                          const char* s2_expression,
                                          const char* s1,
                                          const char* s2);

GTEST_API_ AssertionResult CmpHelperSTRCASENE(const char* s1_expression,
                                              const char* s2_expression,
                                              const char* s1,
                                              const char* s2);


GTEST_API_ AssertionResult CmpHelperSTREQ(const char* expected_expression,
                                          const char* actual_expression,
                                          const wchar_t* expected,
                                          const wchar_t* actual);

GTEST_API_ AssertionResult CmpHelperSTRNE(const char* s1_expression,
                                          const char* s2_expression,
                                          const wchar_t* s1,
                                          const wchar_t* s2);

}  // namespace internal

GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack);
GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack);
GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack);

#if GTEST_HAS_STD_WSTRING
GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack);
#endif  // GTEST_HAS_STD_WSTRING

namespace internal {

template <typename RawType>
AssertionResult CmpHelperFloatingPointEQ(const char* expected_expression,
                                         const char* actual_expression,
                                         RawType expected,
                                         RawType actual) {
  const FloatingPoint<RawType> lhs(expected), rhs(actual);

  if (lhs.AlmostEquals(rhs)) {
    return AssertionSuccess();
  }

  ::std::stringstream expected_ss;
  expected_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
              << expected;

  ::std::stringstream actual_ss;
  actual_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
            << actual;

  return EqFailure(expected_expression,
                   actual_expression,
                   StringStreamToString(&expected_ss),
                   StringStreamToString(&actual_ss),
                   false);
}

GTEST_API_ AssertionResult DoubleNearPredFormat(const char* expr1,
                                                const char* expr2,
                                                const char* abs_error_expr,
                                                double val1,
                                                double val2,
                                                double abs_error);

class GTEST_API_ AssertHelper {
 public:
  AssertHelper(TestPartResult::Type type,
               const char* file,
               int line,
               const char* message);
  ~AssertHelper();

  void operator=(const Message& message) const;

 private:
  struct AssertHelperData {
    AssertHelperData(TestPartResult::Type t,
                     const char* srcfile,
                     int line_num,
                     const char* msg)
        : type(t), file(srcfile), line(line_num), message(msg) { }

    TestPartResult::Type const type;
    const char*        const file;
    int                const line;
    String             const message;

   private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(AssertHelperData);
  };

  AssertHelperData* const data_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(AssertHelper);
};

}  // namespace internal

#if GTEST_HAS_PARAM_TEST

template <typename T>
class WithParamInterface {
 public:
  typedef T ParamType;
  virtual ~WithParamInterface() {}

  const ParamType& GetParam() const { return *parameter_; }

 private:
  static void SetParam(const ParamType* parameter) {
    parameter_ = parameter;
  }

  static const ParamType* parameter_;

  template <class TestClass> friend class internal::ParameterizedTestFactory;
};

template <typename T>
const T* WithParamInterface<T>::parameter_ = NULL;


template <typename T>
class TestWithParam : public Test, public WithParamInterface<T> {
};

#endif  // GTEST_HAS_PARAM_TEST



#define ADD_FAILURE() GTEST_NONFATAL_FAILURE_("Failed")

#define ADD_FAILURE_AT(file, line) \
  GTEST_MESSAGE_AT_(file, line, "Failed", \
                    ::testing::TestPartResult::kNonFatalFailure)

#define GTEST_FAIL() GTEST_FATAL_FAILURE_("Failed")

#if !GTEST_DONT_DEFINE_FAIL
# define FAIL() GTEST_FAIL()
#endif

#define GTEST_SUCCEED() GTEST_SUCCESS_("Succeeded")

#if !GTEST_DONT_DEFINE_SUCCEED
# define SUCCEED() GTEST_SUCCEED()
#endif


#define EXPECT_THROW(statement, expected_exception) \
  GTEST_TEST_THROW_(statement, expected_exception, GTEST_NONFATAL_FAILURE_)
#define EXPECT_NO_THROW(statement) \
  GTEST_TEST_NO_THROW_(statement, GTEST_NONFATAL_FAILURE_)
#define EXPECT_ANY_THROW(statement) \
  GTEST_TEST_ANY_THROW_(statement, GTEST_NONFATAL_FAILURE_)
#define ASSERT_THROW(statement, expected_exception) \
  GTEST_TEST_THROW_(statement, expected_exception, GTEST_FATAL_FAILURE_)
#define ASSERT_NO_THROW(statement) \
  GTEST_TEST_NO_THROW_(statement, GTEST_FATAL_FAILURE_)
#define ASSERT_ANY_THROW(statement) \
  GTEST_TEST_ANY_THROW_(statement, GTEST_FATAL_FAILURE_)

#define EXPECT_TRUE(condition) \
  GTEST_TEST_BOOLEAN_(condition, #condition, false, true, \
                      GTEST_NONFATAL_FAILURE_)
#define EXPECT_FALSE(condition) \
  GTEST_TEST_BOOLEAN_(!(condition), #condition, true, false, \
                      GTEST_NONFATAL_FAILURE_)
#define ASSERT_TRUE(condition) \
  GTEST_TEST_BOOLEAN_(condition, #condition, false, true, \
                      GTEST_FATAL_FAILURE_)
#define ASSERT_FALSE(condition) \
  GTEST_TEST_BOOLEAN_(!(condition), #condition, true, false, \
                      GTEST_FATAL_FAILURE_)

#include "gtest/gtest_pred_impl.h"


#define EXPECT_EQ(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal:: \
                      EqHelper<GTEST_IS_NULL_LITERAL_(expected)>::Compare, \
                      expected, actual)
#define EXPECT_NE(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperNE, expected, actual)
#define EXPECT_LE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperLE, val1, val2)
#define EXPECT_LT(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperLT, val1, val2)
#define EXPECT_GE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperGE, val1, val2)
#define EXPECT_GT(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperGT, val1, val2)

#define GTEST_ASSERT_EQ(expected, actual) \
  ASSERT_PRED_FORMAT2(::testing::internal:: \
                      EqHelper<GTEST_IS_NULL_LITERAL_(expected)>::Compare, \
                      expected, actual)
#define GTEST_ASSERT_NE(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperNE, val1, val2)
#define GTEST_ASSERT_LE(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperLE, val1, val2)
#define GTEST_ASSERT_LT(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperLT, val1, val2)
#define GTEST_ASSERT_GE(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperGE, val1, val2)
#define GTEST_ASSERT_GT(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperGT, val1, val2)


#if !GTEST_DONT_DEFINE_ASSERT_EQ
# define ASSERT_EQ(val1, val2) GTEST_ASSERT_EQ(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_NE
# define ASSERT_NE(val1, val2) GTEST_ASSERT_NE(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_LE
# define ASSERT_LE(val1, val2) GTEST_ASSERT_LE(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_LT
# define ASSERT_LT(val1, val2) GTEST_ASSERT_LT(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_GE
# define ASSERT_GE(val1, val2) GTEST_ASSERT_GE(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_GT
# define ASSERT_GT(val1, val2) GTEST_ASSERT_GT(val1, val2)
#endif


#define EXPECT_STREQ(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTREQ, expected, actual)
#define EXPECT_STRNE(s1, s2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTRNE, s1, s2)
#define EXPECT_STRCASEEQ(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASEEQ, expected, actual)
#define EXPECT_STRCASENE(s1, s2)\
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASENE, s1, s2)

#define ASSERT_STREQ(expected, actual) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTREQ, expected, actual)
#define ASSERT_STRNE(s1, s2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTRNE, s1, s2)
#define ASSERT_STRCASEEQ(expected, actual) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASEEQ, expected, actual)
#define ASSERT_STRCASENE(s1, s2)\
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASENE, s1, s2)


#define EXPECT_FLOAT_EQ(expected, actual)\
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<float>, \
                      expected, actual)

#define EXPECT_DOUBLE_EQ(expected, actual)\
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<double>, \
                      expected, actual)

#define ASSERT_FLOAT_EQ(expected, actual)\
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<float>, \
                      expected, actual)

#define ASSERT_DOUBLE_EQ(expected, actual)\
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<double>, \
                      expected, actual)

#define EXPECT_NEAR(val1, val2, abs_error)\
  EXPECT_PRED_FORMAT3(::testing::internal::DoubleNearPredFormat, \
                      val1, val2, abs_error)

#define ASSERT_NEAR(val1, val2, abs_error)\
  ASSERT_PRED_FORMAT3(::testing::internal::DoubleNearPredFormat, \
                      val1, val2, abs_error)


GTEST_API_ AssertionResult FloatLE(const char* expr1, const char* expr2,
                                   float val1, float val2);
GTEST_API_ AssertionResult DoubleLE(const char* expr1, const char* expr2,
                                    double val1, double val2);


#if GTEST_OS_WINDOWS

# define EXPECT_HRESULT_SUCCEEDED(expr) \
    EXPECT_PRED_FORMAT1(::testing::internal::IsHRESULTSuccess, (expr))

# define ASSERT_HRESULT_SUCCEEDED(expr) \
    ASSERT_PRED_FORMAT1(::testing::internal::IsHRESULTSuccess, (expr))

# define EXPECT_HRESULT_FAILED(expr) \
    EXPECT_PRED_FORMAT1(::testing::internal::IsHRESULTFailure, (expr))

# define ASSERT_HRESULT_FAILED(expr) \
    ASSERT_PRED_FORMAT1(::testing::internal::IsHRESULTFailure, (expr))

#endif  // GTEST_OS_WINDOWS

#define ASSERT_NO_FATAL_FAILURE(statement) \
    GTEST_TEST_NO_FATAL_FAILURE_(statement, GTEST_FATAL_FAILURE_)
#define EXPECT_NO_FATAL_FAILURE(statement) \
    GTEST_TEST_NO_FATAL_FAILURE_(statement, GTEST_NONFATAL_FAILURE_)

#define SCOPED_TRACE(message) \
  ::testing::internal::ScopedTrace GTEST_CONCAT_TOKEN_(gtest_trace_, __LINE__)(\
    __FILE__, __LINE__, ::testing::Message() << (message))

template <typename T1, typename T2>
bool StaticAssertTypeEq() {
  (void)internal::StaticAssertTypeEqHelper<T1, T2>();
  return true;
}


#define GTEST_TEST(test_case_name, test_name)\
  GTEST_TEST_(test_case_name, test_name, \
              ::testing::Test, ::testing::internal::GetTestTypeId())

#if !GTEST_DONT_DEFINE_TEST
# define TEST(test_case_name, test_name) GTEST_TEST(test_case_name, test_name)
#endif


#define TEST_F(test_fixture, test_name)\
  GTEST_TEST_(test_fixture, test_name, test_fixture, \
              ::testing::internal::GetTypeId<test_fixture>())


#define RUN_ALL_TESTS()\
  (::testing::UnitTest::GetInstance()->Run())

}  // namespace testing

#endif  // GTEST_INCLUDE_GTEST_GTEST_H_
