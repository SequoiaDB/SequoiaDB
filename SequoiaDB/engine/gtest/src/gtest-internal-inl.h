

#ifndef GTEST_SRC_GTEST_INTERNAL_INL_H_
#define GTEST_SRC_GTEST_INTERNAL_INL_H_

#if !GTEST_IMPLEMENTATION_
# error "gtest-internal-inl.h is part of Google Test's internal implementation."
# error "It must not be included except by Google Test itself."
#endif  // GTEST_IMPLEMENTATION_

#ifndef _WIN32_WCE
# include <errno.h>
#endif  // !_WIN32_WCE
#include <stddef.h>
#include <stdlib.h>  // For strtoll/_strtoul64/malloc/free.
#include <string.h>  // For memmove.

#include <algorithm>
#include <string>
#include <vector>

#include "gtest/internal/gtest-port.h"

#if GTEST_OS_WINDOWS
# include <windows.h>  // NOLINT
#endif  // GTEST_OS_WINDOWS

#include "gtest/gtest.h"  // NOLINT
#include "gtest/gtest-spi.h"

namespace testing {

GTEST_DECLARE_bool_(death_test_use_fork);

namespace internal {

GTEST_API_ extern const TypeId kTestTypeIdInGoogleTest;

const char kAlsoRunDisabledTestsFlag[] = "also_run_disabled_tests";
const char kBreakOnFailureFlag[] = "break_on_failure";
const char kCatchExceptionsFlag[] = "catch_exceptions";
const char kColorFlag[] = "color";
const char kFilterFlag[] = "filter";
const char kListTestsFlag[] = "list_tests";
const char kOutputFlag[] = "output";
const char kPrintTimeFlag[] = "print_time";
const char kRandomSeedFlag[] = "random_seed";
const char kRepeatFlag[] = "repeat";
const char kShuffleFlag[] = "shuffle";
const char kStackTraceDepthFlag[] = "stack_trace_depth";
const char kStreamResultToFlag[] = "stream_result_to";
const char kThrowOnFailureFlag[] = "throw_on_failure";

const int kMaxRandomSeed = 99999;

GTEST_API_ extern bool g_help_flag;

GTEST_API_ TimeInMillis GetTimeInMillis();

GTEST_API_ bool ShouldUseColor(bool stdout_is_tty);

GTEST_API_ std::string FormatTimeInMillisAsSeconds(TimeInMillis ms);

GTEST_API_ bool ParseInt32Flag(
    const char* str, const char* flag, Int32* value);

inline int GetRandomSeedFromFlag(Int32 random_seed_flag) {
  const unsigned int raw_seed = (random_seed_flag == 0) ?
      static_cast<unsigned int>(GetTimeInMillis()) :
      static_cast<unsigned int>(random_seed_flag);

  const int normalized_seed =
      static_cast<int>((raw_seed - 1U) %
                       static_cast<unsigned int>(kMaxRandomSeed)) + 1;
  return normalized_seed;
}

inline int GetNextRandomSeed(int seed) {
  GTEST_CHECK_(1 <= seed && seed <= kMaxRandomSeed)
      << "Invalid random seed " << seed << " - must be in [1, "
      << kMaxRandomSeed << "].";
  const int next_seed = seed + 1;
  return (next_seed > kMaxRandomSeed) ? 1 : next_seed;
}

class GTestFlagSaver {
 public:
  GTestFlagSaver() {
    also_run_disabled_tests_ = GTEST_FLAG(also_run_disabled_tests);
    break_on_failure_ = GTEST_FLAG(break_on_failure);
    catch_exceptions_ = GTEST_FLAG(catch_exceptions);
    color_ = GTEST_FLAG(color);
    death_test_style_ = GTEST_FLAG(death_test_style);
    death_test_use_fork_ = GTEST_FLAG(death_test_use_fork);
    filter_ = GTEST_FLAG(filter);
    internal_run_death_test_ = GTEST_FLAG(internal_run_death_test);
    list_tests_ = GTEST_FLAG(list_tests);
    output_ = GTEST_FLAG(output);
    print_time_ = GTEST_FLAG(print_time);
    random_seed_ = GTEST_FLAG(random_seed);
    repeat_ = GTEST_FLAG(repeat);
    shuffle_ = GTEST_FLAG(shuffle);
    stack_trace_depth_ = GTEST_FLAG(stack_trace_depth);
    stream_result_to_ = GTEST_FLAG(stream_result_to);
    throw_on_failure_ = GTEST_FLAG(throw_on_failure);
  }

  ~GTestFlagSaver() {
    GTEST_FLAG(also_run_disabled_tests) = also_run_disabled_tests_;
    GTEST_FLAG(break_on_failure) = break_on_failure_;
    GTEST_FLAG(catch_exceptions) = catch_exceptions_;
    GTEST_FLAG(color) = color_;
    GTEST_FLAG(death_test_style) = death_test_style_;
    GTEST_FLAG(death_test_use_fork) = death_test_use_fork_;
    GTEST_FLAG(filter) = filter_;
    GTEST_FLAG(internal_run_death_test) = internal_run_death_test_;
    GTEST_FLAG(list_tests) = list_tests_;
    GTEST_FLAG(output) = output_;
    GTEST_FLAG(print_time) = print_time_;
    GTEST_FLAG(random_seed) = random_seed_;
    GTEST_FLAG(repeat) = repeat_;
    GTEST_FLAG(shuffle) = shuffle_;
    GTEST_FLAG(stack_trace_depth) = stack_trace_depth_;
    GTEST_FLAG(stream_result_to) = stream_result_to_;
    GTEST_FLAG(throw_on_failure) = throw_on_failure_;
  }
 private:
  bool also_run_disabled_tests_;
  bool break_on_failure_;
  bool catch_exceptions_;
  String color_;
  String death_test_style_;
  bool death_test_use_fork_;
  String filter_;
  String internal_run_death_test_;
  bool list_tests_;
  String output_;
  bool print_time_;
  bool pretty_;
  internal::Int32 random_seed_;
  internal::Int32 repeat_;
  bool shuffle_;
  internal::Int32 stack_trace_depth_;
  String stream_result_to_;
  bool throw_on_failure_;
} GTEST_ATTRIBUTE_UNUSED_;

GTEST_API_ char* CodePointToUtf8(UInt32 code_point, char* str);

GTEST_API_ String WideStringToUtf8(const wchar_t* str, int num_chars);

void WriteToShardStatusFileIfNeeded();

GTEST_API_ bool ShouldShard(const char* total_shards_str,
                            const char* shard_index_str,
                            bool in_subprocess_for_death_test);

GTEST_API_ Int32 Int32FromEnvOrDie(const char* env_var, Int32 default_val);

GTEST_API_ bool ShouldRunTestOnShard(
    int total_shards, int shard_index, int test_id);


template <class Container, typename Predicate>
inline int CountIf(const Container& c, Predicate predicate) {
  int count = 0;
  for (typename Container::const_iterator it = c.begin(); it != c.end(); ++it) {
    if (predicate(*it))
      ++count;
  }
  return count;
}

template <class Container, typename Functor>
void ForEach(const Container& c, Functor functor) {
  std::for_each(c.begin(), c.end(), functor);
}

template <typename E>
inline E GetElementOr(const std::vector<E>& v, int i, E default_value) {
  return (i < 0 || i >= static_cast<int>(v.size())) ? default_value : v[i];
}

template <typename E>
void ShuffleRange(internal::Random* random, int begin, int end,
                  std::vector<E>* v) {
  const int size = static_cast<int>(v->size());
  GTEST_CHECK_(0 <= begin && begin <= size)
      << "Invalid shuffle range start " << begin << ": must be in range [0, "
      << size << "].";
  GTEST_CHECK_(begin <= end && end <= size)
      << "Invalid shuffle range finish " << end << ": must be in range ["
      << begin << ", " << size << "].";

  for (int range_width = end - begin; range_width >= 2; range_width--) {
    const int last_in_range = begin + range_width - 1;
    const int selected = begin + random->Generate(range_width);
    std::swap((*v)[selected], (*v)[last_in_range]);
  }
}

template <typename E>
inline void Shuffle(internal::Random* random, std::vector<E>* v) {
  ShuffleRange(random, 0, static_cast<int>(v->size()), v);
}

template <typename T>
static void Delete(T* x) {
  delete x;
}

class TestPropertyKeyIs {
 public:
  explicit TestPropertyKeyIs(const char* key)
      : key_(key) {}

  bool operator()(const TestProperty& test_property) const {
    return String(test_property.key()).Compare(key_) == 0;
  }

 private:
  String key_;
};

class GTEST_API_ UnitTestOptions {
 public:

  static String GetOutputFormat();

  static String GetAbsolutePathToOutputFile();


  static bool PatternMatchesString(const char *pattern, const char *str);

  static bool FilterMatchesTest(const String &test_case_name,
                                const String &test_name);

#if GTEST_OS_WINDOWS

  static int GTestShouldProcessSEH(DWORD exception_code);
#endif  // GTEST_OS_WINDOWS

  static bool MatchesFilter(const String& name, const char* filter);
};

GTEST_API_ FilePath GetCurrentExecutableName();

class OsStackTraceGetterInterface {
 public:
  OsStackTraceGetterInterface() {}
  virtual ~OsStackTraceGetterInterface() {}

  virtual String CurrentStackTrace(int max_depth, int skip_count) = 0;

  virtual void UponLeavingGTest() = 0;

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(OsStackTraceGetterInterface);
};

class OsStackTraceGetter : public OsStackTraceGetterInterface {
 public:
  OsStackTraceGetter() : caller_frame_(NULL) {}
  virtual String CurrentStackTrace(int max_depth, int skip_count);
  virtual void UponLeavingGTest();

  static const char* const kElidedFramesMarker;

 private:
  Mutex mutex_;  // protects all internal state

  void* caller_frame_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(OsStackTraceGetter);
};

struct TraceInfo {
  const char* file;
  int line;
  String message;
};

class DefaultGlobalTestPartResultReporter
  : public TestPartResultReporterInterface {
 public:
  explicit DefaultGlobalTestPartResultReporter(UnitTestImpl* unit_test);
  virtual void ReportTestPartResult(const TestPartResult& result);

 private:
  UnitTestImpl* const unit_test_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(DefaultGlobalTestPartResultReporter);
};

class DefaultPerThreadTestPartResultReporter
    : public TestPartResultReporterInterface {
 public:
  explicit DefaultPerThreadTestPartResultReporter(UnitTestImpl* unit_test);
  virtual void ReportTestPartResult(const TestPartResult& result);

 private:
  UnitTestImpl* const unit_test_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(DefaultPerThreadTestPartResultReporter);
};

class GTEST_API_ UnitTestImpl {
 public:
  explicit UnitTestImpl(UnitTest* parent);
  virtual ~UnitTestImpl();


  TestPartResultReporterInterface* GetGlobalTestPartResultReporter();

  void SetGlobalTestPartResultReporter(
      TestPartResultReporterInterface* reporter);

  TestPartResultReporterInterface* GetTestPartResultReporterForCurrentThread();

  void SetTestPartResultReporterForCurrentThread(
      TestPartResultReporterInterface* reporter);

  int successful_test_case_count() const;

  int failed_test_case_count() const;

  int total_test_case_count() const;

  int test_case_to_run_count() const;

  int successful_test_count() const;

  int failed_test_count() const;

  int disabled_test_count() const;

  int total_test_count() const;

  int test_to_run_count() const;

  TimeInMillis elapsed_time() const { return elapsed_time_; }

  bool Passed() const { return !Failed(); }

  bool Failed() const {
    return failed_test_case_count() > 0 || ad_hoc_test_result()->Failed();
  }

  const TestCase* GetTestCase(int i) const {
    const int index = GetElementOr(test_case_indices_, i, -1);
    return index < 0 ? NULL : test_cases_[i];
  }

  TestCase* GetMutableTestCase(int i) {
    const int index = GetElementOr(test_case_indices_, i, -1);
    return index < 0 ? NULL : test_cases_[index];
  }

  TestEventListeners* listeners() { return &listeners_; }

  TestResult* current_test_result();

  const TestResult* ad_hoc_test_result() const { return &ad_hoc_test_result_; }

  void set_os_stack_trace_getter(OsStackTraceGetterInterface* getter);

  OsStackTraceGetterInterface* os_stack_trace_getter();

  String CurrentOsStackTraceExceptTop(int skip_count);

  TestCase* GetTestCase(const char* test_case_name,
                        const char* type_param,
                        Test::SetUpTestCaseFunc set_up_tc,
                        Test::TearDownTestCaseFunc tear_down_tc);

  void AddTestInfo(Test::SetUpTestCaseFunc set_up_tc,
                   Test::TearDownTestCaseFunc tear_down_tc,
                   TestInfo* test_info) {
    if (original_working_dir_.IsEmpty()) {
      original_working_dir_.Set(FilePath::GetCurrentDir());
      GTEST_CHECK_(!original_working_dir_.IsEmpty())
          << "Failed to get the current working directory.";
    }

    GetTestCase(test_info->test_case_name(),
                test_info->type_param(),
                set_up_tc,
                tear_down_tc)->AddTestInfo(test_info);
  }

#if GTEST_HAS_PARAM_TEST
  internal::ParameterizedTestCaseRegistry& parameterized_test_registry() {
    return parameterized_test_registry_;
  }
#endif  // GTEST_HAS_PARAM_TEST

  void set_current_test_case(TestCase* a_current_test_case) {
    current_test_case_ = a_current_test_case;
  }

  void set_current_test_info(TestInfo* a_current_test_info) {
    current_test_info_ = a_current_test_info;
  }

  void RegisterParameterizedTests();

  bool RunAllTests();

  void ClearNonAdHocTestResult() {
    ForEach(test_cases_, TestCase::ClearTestCaseResult);
  }

  void ClearAdHocTestResult() {
    ad_hoc_test_result_.Clear();
  }

  enum ReactionToSharding {
    HONOR_SHARDING_PROTOCOL,
    IGNORE_SHARDING_PROTOCOL
  };

  int FilterTests(ReactionToSharding shard_tests);

  void ListTestsMatchingFilter();

  const TestCase* current_test_case() const { return current_test_case_; }
  TestInfo* current_test_info() { return current_test_info_; }
  const TestInfo* current_test_info() const { return current_test_info_; }

  std::vector<Environment*>& environments() { return environments_; }

  std::vector<TraceInfo>& gtest_trace_stack() {
    return *(gtest_trace_stack_.pointer());
  }
  const std::vector<TraceInfo>& gtest_trace_stack() const {
    return gtest_trace_stack_.get();
  }

#if GTEST_HAS_DEATH_TEST
  void InitDeathTestSubprocessControlInfo() {
    internal_run_death_test_flag_.reset(ParseInternalRunDeathTestFlag());
  }
  const InternalRunDeathTestFlag* internal_run_death_test_flag() const {
    return internal_run_death_test_flag_.get();
  }

  internal::DeathTestFactory* death_test_factory() {
    return death_test_factory_.get();
  }

  void SuppressTestEventsIfInSubprocess();

  friend class ReplaceDeathTestFactory;
#endif  // GTEST_HAS_DEATH_TEST

  void ConfigureXmlOutput();

#if GTEST_CAN_STREAM_RESULTS_
  void ConfigureStreamingOutput();
#endif

  void PostFlagParsingInit();

  int random_seed() const { return random_seed_; }

  internal::Random* random() { return &random_; }

  void ShuffleTests();

  void UnshuffleTests();

  bool catch_exceptions() const { return catch_exceptions_; }

 private:
  friend class ::testing::UnitTest;

  void set_catch_exceptions(bool value) { catch_exceptions_ = value; }

  UnitTest* const parent_;

  internal::FilePath original_working_dir_;

  DefaultGlobalTestPartResultReporter default_global_test_part_result_reporter_;
  DefaultPerThreadTestPartResultReporter
      default_per_thread_test_part_result_reporter_;

  TestPartResultReporterInterface* global_test_part_result_repoter_;

  internal::Mutex global_test_part_result_reporter_mutex_;

  internal::ThreadLocal<TestPartResultReporterInterface*>
      per_thread_test_part_result_reporter_;

  std::vector<Environment*> environments_;

  std::vector<TestCase*> test_cases_;

  std::vector<int> test_case_indices_;

#if GTEST_HAS_PARAM_TEST
  internal::ParameterizedTestCaseRegistry parameterized_test_registry_;

  bool parameterized_tests_registered_;
#endif  // GTEST_HAS_PARAM_TEST

  int last_death_test_case_;

  TestCase* current_test_case_;

  TestInfo* current_test_info_;

  TestResult ad_hoc_test_result_;

  TestEventListeners listeners_;

  OsStackTraceGetterInterface* os_stack_trace_getter_;

  bool post_flag_parse_init_performed_;

  int random_seed_;

  internal::Random random_;

  TimeInMillis elapsed_time_;

#if GTEST_HAS_DEATH_TEST
  internal::scoped_ptr<InternalRunDeathTestFlag> internal_run_death_test_flag_;
  internal::scoped_ptr<internal::DeathTestFactory> death_test_factory_;
#endif  // GTEST_HAS_DEATH_TEST

  internal::ThreadLocal<std::vector<TraceInfo> > gtest_trace_stack_;

  bool catch_exceptions_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(UnitTestImpl);
};  // class UnitTestImpl

inline UnitTestImpl* GetUnitTestImpl() {
  return UnitTest::GetInstance()->impl();
}

#if GTEST_USES_SIMPLE_RE

GTEST_API_ bool IsInSet(char ch, const char* str);
GTEST_API_ bool IsAsciiDigit(char ch);
GTEST_API_ bool IsAsciiPunct(char ch);
GTEST_API_ bool IsRepeat(char ch);
GTEST_API_ bool IsAsciiWhiteSpace(char ch);
GTEST_API_ bool IsAsciiWordChar(char ch);
GTEST_API_ bool IsValidEscape(char ch);
GTEST_API_ bool AtomMatchesChar(bool escaped, char pattern, char ch);
GTEST_API_ bool ValidateRegex(const char* regex);
GTEST_API_ bool MatchRegexAtHead(const char* regex, const char* str);
GTEST_API_ bool MatchRepetitionAndRegexAtHead(
    bool escaped, char ch, char repeat, const char* regex, const char* str);
GTEST_API_ bool MatchRegexAnywhere(const char* regex, const char* str);

#endif  // GTEST_USES_SIMPLE_RE

GTEST_API_ void ParseGoogleTestFlagsOnly(int* argc, char** argv);
GTEST_API_ void ParseGoogleTestFlagsOnly(int* argc, wchar_t** argv);

#if GTEST_HAS_DEATH_TEST

GTEST_API_ String GetLastErrnoDescription();

# if GTEST_OS_WINDOWS
class AutoHandle {
 public:
  AutoHandle() : handle_(INVALID_HANDLE_VALUE) {}
  explicit AutoHandle(HANDLE handle) : handle_(handle) {}

  ~AutoHandle() { Reset(); }

  HANDLE Get() const { return handle_; }
  void Reset() { Reset(INVALID_HANDLE_VALUE); }
  void Reset(HANDLE handle) {
    if (handle != handle_) {
      if (handle_ != INVALID_HANDLE_VALUE)
        ::CloseHandle(handle_);
      handle_ = handle;
    }
  }

 private:
  HANDLE handle_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(AutoHandle);
};
# endif  // GTEST_OS_WINDOWS

template <typename Integer>
bool ParseNaturalNumber(const ::std::string& str, Integer* number) {
  if (str.empty() || !IsDigit(str[0])) {
    return false;
  }
  errno = 0;

  char* end;

# if GTEST_OS_WINDOWS && !defined(__GNUC__)

  typedef unsigned __int64 BiggestConvertible;
  const BiggestConvertible parsed = _strtoui64(str.c_str(), &end, 10);

# else

  typedef unsigned long long BiggestConvertible;  // NOLINT
  const BiggestConvertible parsed = strtoull(str.c_str(), &end, 10);

# endif  // GTEST_OS_WINDOWS && !defined(__GNUC__)

  const bool parse_success = *end == '\0' && errno == 0;

  GTEST_CHECK_(sizeof(Integer) <= sizeof(parsed));

  const Integer result = static_cast<Integer>(parsed);
  if (parse_success && static_cast<BiggestConvertible>(result) == parsed) {
    *number = result;
    return true;
  }
  return false;
}
#endif  // GTEST_HAS_DEATH_TEST

class TestResultAccessor {
 public:
  static void RecordProperty(TestResult* test_result,
                             const TestProperty& property) {
    test_result->RecordProperty(property);
  }

  static void ClearTestPartResults(TestResult* test_result) {
    test_result->ClearTestPartResults();
  }

  static const std::vector<testing::TestPartResult>& test_part_results(
      const TestResult& test_result) {
    return test_result.test_part_results();
  }
};

}  // namespace internal
}  // namespace testing

#endif  // GTEST_SRC_GTEST_INTERNAL_INL_H_
