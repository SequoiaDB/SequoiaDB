

#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PARAM_UTIL_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PARAM_UTIL_H_

#include <iterator>
#include <utility>
#include <vector>

#include "gtest/internal/gtest-internal.h"
#include "gtest/internal/gtest-linked_ptr.h"
#include "gtest/internal/gtest-port.h"
#include "gtest/gtest-printers.h"

#if GTEST_HAS_PARAM_TEST

namespace testing {
namespace internal {

GTEST_API_ void ReportInvalidTestCaseType(const char* test_case_name,
                                          const char* file, int line);

template <typename> class ParamGeneratorInterface;
template <typename> class ParamGenerator;

template <typename T>
class ParamIteratorInterface {
 public:
  virtual ~ParamIteratorInterface() {}
  virtual const ParamGeneratorInterface<T>* BaseGenerator() const = 0;
  virtual void Advance() = 0;
  virtual ParamIteratorInterface* Clone() const = 0;
  virtual const T* Current() const = 0;
  virtual bool Equals(const ParamIteratorInterface& other) const = 0;
};

template <typename T>
class ParamIterator {
 public:
  typedef T value_type;
  typedef const T& reference;
  typedef ptrdiff_t difference_type;

  ParamIterator(const ParamIterator& other) : impl_(other.impl_->Clone()) {}
  ParamIterator& operator=(const ParamIterator& other) {
    if (this != &other)
      impl_.reset(other.impl_->Clone());
    return *this;
  }

  const T& operator*() const { return *impl_->Current(); }
  const T* operator->() const { return impl_->Current(); }
  ParamIterator& operator++() {
    impl_->Advance();
    return *this;
  }
  ParamIterator operator++(int /*unused*/) {
    ParamIteratorInterface<T>* clone = impl_->Clone();
    impl_->Advance();
    return ParamIterator(clone);
  }
  bool operator==(const ParamIterator& other) const {
    return impl_.get() == other.impl_.get() || impl_->Equals(*other.impl_);
  }
  bool operator!=(const ParamIterator& other) const {
    return !(*this == other);
  }

 private:
  friend class ParamGenerator<T>;
  explicit ParamIterator(ParamIteratorInterface<T>* impl) : impl_(impl) {}
  scoped_ptr<ParamIteratorInterface<T> > impl_;
};

template <typename T>
class ParamGeneratorInterface {
 public:
  typedef T ParamType;

  virtual ~ParamGeneratorInterface() {}

  virtual ParamIteratorInterface<T>* Begin() const = 0;
  virtual ParamIteratorInterface<T>* End() const = 0;
};

template<typename T>
class ParamGenerator {
 public:
  typedef ParamIterator<T> iterator;

  explicit ParamGenerator(ParamGeneratorInterface<T>* impl) : impl_(impl) {}
  ParamGenerator(const ParamGenerator& other) : impl_(other.impl_) {}

  ParamGenerator& operator=(const ParamGenerator& other) {
    impl_ = other.impl_;
    return *this;
  }

  iterator begin() const { return iterator(impl_->Begin()); }
  iterator end() const { return iterator(impl_->End()); }

 private:
  linked_ptr<const ParamGeneratorInterface<T> > impl_;
};

template <typename T, typename IncrementT>
class RangeGenerator : public ParamGeneratorInterface<T> {
 public:
  RangeGenerator(T begin, T end, IncrementT step)
      : begin_(begin), end_(end),
        step_(step), end_index_(CalculateEndIndex(begin, end, step)) {}
  virtual ~RangeGenerator() {}

  virtual ParamIteratorInterface<T>* Begin() const {
    return new Iterator(this, begin_, 0, step_);
  }
  virtual ParamIteratorInterface<T>* End() const {
    return new Iterator(this, end_, end_index_, step_);
  }

 private:
  class Iterator : public ParamIteratorInterface<T> {
   public:
    Iterator(const ParamGeneratorInterface<T>* base, T value, int index,
             IncrementT step)
        : base_(base), value_(value), index_(index), step_(step) {}
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<T>* BaseGenerator() const {
      return base_;
    }
    virtual void Advance() {
      value_ = value_ + step_;
      index_++;
    }
    virtual ParamIteratorInterface<T>* Clone() const {
      return new Iterator(*this);
    }
    virtual const T* Current() const { return &value_; }
    virtual bool Equals(const ParamIteratorInterface<T>& other) const {
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const int other_index =
          CheckedDowncastToActualType<const Iterator>(&other)->index_;
      return index_ == other_index;
    }

   private:
    Iterator(const Iterator& other)
        : ParamIteratorInterface<T>(),
          base_(other.base_), value_(other.value_), index_(other.index_),
          step_(other.step_) {}

    void operator=(const Iterator& other);

    const ParamGeneratorInterface<T>* const base_;
    T value_;
    int index_;
    const IncrementT step_;
  };  // class RangeGenerator::Iterator

  static int CalculateEndIndex(const T& begin,
                               const T& end,
                               const IncrementT& step) {
    int end_index = 0;
    for (T i = begin; i < end; i = i + step)
      end_index++;
    return end_index;
  }

  void operator=(const RangeGenerator& other);

  const T begin_;
  const T end_;
  const IncrementT step_;
  const int end_index_;
};  // class RangeGenerator


template <typename T>
class ValuesInIteratorRangeGenerator : public ParamGeneratorInterface<T> {
 public:
  template <typename ForwardIterator>
  ValuesInIteratorRangeGenerator(ForwardIterator begin, ForwardIterator end)
      : container_(begin, end) {}
  virtual ~ValuesInIteratorRangeGenerator() {}

  virtual ParamIteratorInterface<T>* Begin() const {
    return new Iterator(this, container_.begin());
  }
  virtual ParamIteratorInterface<T>* End() const {
    return new Iterator(this, container_.end());
  }

 private:
  typedef typename ::std::vector<T> ContainerType;

  class Iterator : public ParamIteratorInterface<T> {
   public:
    Iterator(const ParamGeneratorInterface<T>* base,
             typename ContainerType::const_iterator iterator)
        : base_(base), iterator_(iterator) {}
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<T>* BaseGenerator() const {
      return base_;
    }
    virtual void Advance() {
      ++iterator_;
      value_.reset();
    }
    virtual ParamIteratorInterface<T>* Clone() const {
      return new Iterator(*this);
    }
    virtual const T* Current() const {
      if (value_.get() == NULL)
        value_.reset(new T(*iterator_));
      return value_.get();
    }
    virtual bool Equals(const ParamIteratorInterface<T>& other) const {
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      return iterator_ ==
          CheckedDowncastToActualType<const Iterator>(&other)->iterator_;
    }

   private:
    Iterator(const Iterator& other)
        : ParamIteratorInterface<T>(),
          base_(other.base_),
          iterator_(other.iterator_) {}

    const ParamGeneratorInterface<T>* const base_;
    typename ContainerType::const_iterator iterator_;
    mutable scoped_ptr<const T> value_;
  };  // class ValuesInIteratorRangeGenerator::Iterator

  void operator=(const ValuesInIteratorRangeGenerator& other);

  const ContainerType container_;
};  // class ValuesInIteratorRangeGenerator

template <class TestClass>
class ParameterizedTestFactory : public TestFactoryBase {
 public:
  typedef typename TestClass::ParamType ParamType;
  explicit ParameterizedTestFactory(ParamType parameter) :
      parameter_(parameter) {}
  virtual Test* CreateTest() {
    TestClass::SetParam(&parameter_);
    return new TestClass();
  }

 private:
  const ParamType parameter_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestFactory);
};

template <class ParamType>
class TestMetaFactoryBase {
 public:
  virtual ~TestMetaFactoryBase() {}

  virtual TestFactoryBase* CreateTestFactory(ParamType parameter) = 0;
};

template <class TestCase>
class TestMetaFactory
    : public TestMetaFactoryBase<typename TestCase::ParamType> {
 public:
  typedef typename TestCase::ParamType ParamType;

  TestMetaFactory() {}

  virtual TestFactoryBase* CreateTestFactory(ParamType parameter) {
    return new ParameterizedTestFactory<TestCase>(parameter);
  }

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestMetaFactory);
};

class ParameterizedTestCaseInfoBase {
 public:
  virtual ~ParameterizedTestCaseInfoBase() {}

  virtual const string& GetTestCaseName() const = 0;
  virtual TypeId GetTestCaseTypeId() const = 0;
  virtual void RegisterTests() = 0;

 protected:
  ParameterizedTestCaseInfoBase() {}

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestCaseInfoBase);
};

template <class TestCase>
class ParameterizedTestCaseInfo : public ParameterizedTestCaseInfoBase {
 public:
  typedef typename TestCase::ParamType ParamType;
  typedef ParamGenerator<ParamType>(GeneratorCreationFunc)();

  explicit ParameterizedTestCaseInfo(const char* name)
      : test_case_name_(name) {}

  virtual const string& GetTestCaseName() const { return test_case_name_; }
  virtual TypeId GetTestCaseTypeId() const { return GetTypeId<TestCase>(); }
  void AddTestPattern(const char* test_case_name,
                      const char* test_base_name,
                      TestMetaFactoryBase<ParamType>* meta_factory) {
    tests_.push_back(linked_ptr<TestInfo>(new TestInfo(test_case_name,
                                                       test_base_name,
                                                       meta_factory)));
  }
  int AddTestCaseInstantiation(const string& instantiation_name,
                               GeneratorCreationFunc* func,
                               const char* /* file */,
                               int /* line */) {
    instantiations_.push_back(::std::make_pair(instantiation_name, func));
    return 0;  // Return value used only to run this method in namespace scope.
  }
  virtual void RegisterTests() {
    for (typename TestInfoContainer::iterator test_it = tests_.begin();
         test_it != tests_.end(); ++test_it) {
      linked_ptr<TestInfo> test_info = *test_it;
      for (typename InstantiationContainer::iterator gen_it =
               instantiations_.begin(); gen_it != instantiations_.end();
               ++gen_it) {
        const string& instantiation_name = gen_it->first;
        ParamGenerator<ParamType> generator((*gen_it->second)());

        Message test_case_name_stream;
        if ( !instantiation_name.empty() )
          test_case_name_stream << instantiation_name << "/";
        test_case_name_stream << test_info->test_case_base_name;

        int i = 0;
        for (typename ParamGenerator<ParamType>::iterator param_it =
                 generator.begin();
             param_it != generator.end(); ++param_it, ++i) {
          Message test_name_stream;
          test_name_stream << test_info->test_base_name << "/" << i;
          MakeAndRegisterTestInfo(
              test_case_name_stream.GetString().c_str(),
              test_name_stream.GetString().c_str(),
              NULL,  // No type parameter.
              PrintToString(*param_it).c_str(),
              GetTestCaseTypeId(),
              TestCase::SetUpTestCase,
              TestCase::TearDownTestCase,
              test_info->test_meta_factory->CreateTestFactory(*param_it));
        }  // for param_it
      }  // for gen_it
    }  // for test_it
  }  // RegisterTests

 private:
  struct TestInfo {
    TestInfo(const char* a_test_case_base_name,
             const char* a_test_base_name,
             TestMetaFactoryBase<ParamType>* a_test_meta_factory) :
        test_case_base_name(a_test_case_base_name),
        test_base_name(a_test_base_name),
        test_meta_factory(a_test_meta_factory) {}

    const string test_case_base_name;
    const string test_base_name;
    const scoped_ptr<TestMetaFactoryBase<ParamType> > test_meta_factory;
  };
  typedef ::std::vector<linked_ptr<TestInfo> > TestInfoContainer;
  typedef ::std::vector<std::pair<string, GeneratorCreationFunc*> >
      InstantiationContainer;

  const string test_case_name_;
  TestInfoContainer tests_;
  InstantiationContainer instantiations_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestCaseInfo);
};  // class ParameterizedTestCaseInfo

class ParameterizedTestCaseRegistry {
 public:
  ParameterizedTestCaseRegistry() {}
  ~ParameterizedTestCaseRegistry() {
    for (TestCaseInfoContainer::iterator it = test_case_infos_.begin();
         it != test_case_infos_.end(); ++it) {
      delete *it;
    }
  }

  template <class TestCase>
  ParameterizedTestCaseInfo<TestCase>* GetTestCasePatternHolder(
      const char* test_case_name,
      const char* file,
      int line) {
    ParameterizedTestCaseInfo<TestCase>* typed_test_info = NULL;
    for (TestCaseInfoContainer::iterator it = test_case_infos_.begin();
         it != test_case_infos_.end(); ++it) {
      if ((*it)->GetTestCaseName() == test_case_name) {
        if ((*it)->GetTestCaseTypeId() != GetTypeId<TestCase>()) {
          ReportInvalidTestCaseType(test_case_name,  file, line);
          posix::Abort();
        } else {
          typed_test_info = CheckedDowncastToActualType<
              ParameterizedTestCaseInfo<TestCase> >(*it);
        }
        break;
      }
    }
    if (typed_test_info == NULL) {
      typed_test_info = new ParameterizedTestCaseInfo<TestCase>(test_case_name);
      test_case_infos_.push_back(typed_test_info);
    }
    return typed_test_info;
  }
  void RegisterTests() {
    for (TestCaseInfoContainer::iterator it = test_case_infos_.begin();
         it != test_case_infos_.end(); ++it) {
      (*it)->RegisterTests();
    }
  }

 private:
  typedef ::std::vector<ParameterizedTestCaseInfoBase*> TestCaseInfoContainer;

  TestCaseInfoContainer test_case_infos_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestCaseRegistry);
};

}  // namespace internal
}  // namespace testing

#endif  //  GTEST_HAS_PARAM_TEST

#endif  // GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PARAM_UTIL_H_
