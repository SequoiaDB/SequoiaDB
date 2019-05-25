
#ifndef _PCRECPP_H
#define _PCRECPP_H



#include <string>
#include "pcre.h"
#include "pcrecpparg.h"   // defines the Arg class
#include "pcre_stringpiece.h"

namespace pcrecpp {

#define PCRE_SET_OR_CLEAR(b, o) \
    if (b) all_options_ |= (o); else all_options_ &= ~(o); \
    return *this

#define PCRE_IS_SET(o)  \
        (all_options_ & o) == o

/***** Compiling regular expressions: the RE class *****/

class PCRECPP_EXP_DEFN RE_Options {
 public:
  RE_Options() : match_limit_(0), match_limit_recursion_(0), all_options_(0) {}

  RE_Options(int option_flags) : match_limit_(0), match_limit_recursion_(0),
                                 all_options_(option_flags) {}

  int match_limit() const { return match_limit_; };
  RE_Options &set_match_limit(int limit) {
    match_limit_ = limit;
    return *this;
  }

  int match_limit_recursion() const { return match_limit_recursion_; };
  RE_Options &set_match_limit_recursion(int limit) {
    match_limit_recursion_ = limit;
    return *this;
  }

  bool caseless() const {
    return PCRE_IS_SET(PCRE_CASELESS);
  }
  RE_Options &set_caseless(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_CASELESS);
  }

  bool multiline() const {
    return PCRE_IS_SET(PCRE_MULTILINE);
  }
  RE_Options &set_multiline(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_MULTILINE);
  }

  bool dotall() const {
    return PCRE_IS_SET(PCRE_DOTALL);
  }
  RE_Options &set_dotall(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_DOTALL);
  }

  bool extended() const {
    return PCRE_IS_SET(PCRE_EXTENDED);
  }
  RE_Options &set_extended(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_EXTENDED);
  }

  bool dollar_endonly() const {
    return PCRE_IS_SET(PCRE_DOLLAR_ENDONLY);
  }
  RE_Options &set_dollar_endonly(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_DOLLAR_ENDONLY);
  }

  bool extra() const {
    return PCRE_IS_SET(PCRE_EXTRA);
  }
  RE_Options &set_extra(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_EXTRA);
  }

  bool ungreedy() const {
    return PCRE_IS_SET(PCRE_UNGREEDY);
  }
  RE_Options &set_ungreedy(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_UNGREEDY);
  }

  bool utf8() const {
    return PCRE_IS_SET(PCRE_UTF8);
  }
  RE_Options &set_utf8(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_UTF8);
  }

  bool no_auto_capture() const {
    return PCRE_IS_SET(PCRE_NO_AUTO_CAPTURE);
  }
  RE_Options &set_no_auto_capture(bool x) {
    PCRE_SET_OR_CLEAR(x, PCRE_NO_AUTO_CAPTURE);
  }

  RE_Options &set_all_options(int opt) {
    all_options_ = opt;
    return *this;
  }
  int all_options() const {
    return all_options_ ;
  }


 private:
  int match_limit_;
  int match_limit_recursion_;
  int all_options_;
};

static inline RE_Options UTF8() {
  return RE_Options().set_utf8(true);
}

static inline RE_Options CASELESS() {
  return RE_Options().set_caseless(true);
}
static inline RE_Options MULTILINE() {
  return RE_Options().set_multiline(true);
}

static inline RE_Options DOTALL() {
  return RE_Options().set_dotall(true);
}

static inline RE_Options EXTENDED() {
  return RE_Options().set_extended(true);
}

class PCRECPP_EXP_DEFN RE {
 public:
  RE(const string& pat) { Init(pat, NULL); }
  RE(const string& pat, const RE_Options& option) { Init(pat, &option); }
  RE(const char* pat) { Init(pat, NULL); }
  RE(const char* pat, const RE_Options& option) { Init(pat, &option); }
  RE(const unsigned char* pat) {
    Init(reinterpret_cast<const char*>(pat), NULL);
  }
  RE(const unsigned char* pat, const RE_Options& option) {
    Init(reinterpret_cast<const char*>(pat), &option);
  }

  RE(const RE& re) { Init(re.pattern_, &re.options_); }
  const RE& operator=(const RE& re) {
    if (this != &re) {
      Cleanup();


      Init(re.pattern_, &re.options_);
    }
    return *this;
  }


  ~RE();

  const string& pattern() const { return pattern_; }

  const string& error() const { return *error_; }

  /***** The useful part: the matching interface *****/


  bool FullMatch(const StringPiece& text,
                 const Arg& ptr1 = no_arg,
                 const Arg& ptr2 = no_arg,
                 const Arg& ptr3 = no_arg,
                 const Arg& ptr4 = no_arg,
                 const Arg& ptr5 = no_arg,
                 const Arg& ptr6 = no_arg,
                 const Arg& ptr7 = no_arg,
                 const Arg& ptr8 = no_arg,
                 const Arg& ptr9 = no_arg,
                 const Arg& ptr10 = no_arg,
                 const Arg& ptr11 = no_arg,
                 const Arg& ptr12 = no_arg,
                 const Arg& ptr13 = no_arg,
                 const Arg& ptr14 = no_arg,
                 const Arg& ptr15 = no_arg,
                 const Arg& ptr16 = no_arg) const;

  bool PartialMatch(const StringPiece& text,
                    const Arg& ptr1 = no_arg,
                    const Arg& ptr2 = no_arg,
                    const Arg& ptr3 = no_arg,
                    const Arg& ptr4 = no_arg,
                    const Arg& ptr5 = no_arg,
                    const Arg& ptr6 = no_arg,
                    const Arg& ptr7 = no_arg,
                    const Arg& ptr8 = no_arg,
                    const Arg& ptr9 = no_arg,
                    const Arg& ptr10 = no_arg,
                    const Arg& ptr11 = no_arg,
                    const Arg& ptr12 = no_arg,
                    const Arg& ptr13 = no_arg,
                    const Arg& ptr14 = no_arg,
                    const Arg& ptr15 = no_arg,
                    const Arg& ptr16 = no_arg) const;

  bool Consume(StringPiece* input,
               const Arg& ptr1 = no_arg,
               const Arg& ptr2 = no_arg,
               const Arg& ptr3 = no_arg,
               const Arg& ptr4 = no_arg,
               const Arg& ptr5 = no_arg,
               const Arg& ptr6 = no_arg,
               const Arg& ptr7 = no_arg,
               const Arg& ptr8 = no_arg,
               const Arg& ptr9 = no_arg,
               const Arg& ptr10 = no_arg,
               const Arg& ptr11 = no_arg,
               const Arg& ptr12 = no_arg,
               const Arg& ptr13 = no_arg,
               const Arg& ptr14 = no_arg,
               const Arg& ptr15 = no_arg,
               const Arg& ptr16 = no_arg) const;

  bool FindAndConsume(StringPiece* input,
                      const Arg& ptr1 = no_arg,
                      const Arg& ptr2 = no_arg,
                      const Arg& ptr3 = no_arg,
                      const Arg& ptr4 = no_arg,
                      const Arg& ptr5 = no_arg,
                      const Arg& ptr6 = no_arg,
                      const Arg& ptr7 = no_arg,
                      const Arg& ptr8 = no_arg,
                      const Arg& ptr9 = no_arg,
                      const Arg& ptr10 = no_arg,
                      const Arg& ptr11 = no_arg,
                      const Arg& ptr12 = no_arg,
                      const Arg& ptr13 = no_arg,
                      const Arg& ptr14 = no_arg,
                      const Arg& ptr15 = no_arg,
                      const Arg& ptr16 = no_arg) const;

  bool Replace(const StringPiece& rewrite,
               string *str) const;

  int GlobalReplace(const StringPiece& rewrite,
                    string *str) const;

  bool Extract(const StringPiece &rewrite,
               const StringPiece &text,
               string *out) const;

  static string QuoteMeta(const StringPiece& unquoted);


  /***** Generic matching interface *****/

  enum Anchor {
    UNANCHORED,         // No anchoring
    ANCHOR_START,       // Anchor at start only
    ANCHOR_BOTH         // Anchor at start and end
  };

  bool DoMatch(const StringPiece& text,
               Anchor anchor,
               int* consumed,
               const Arg* const* args, int n) const;

  int NumberOfCapturingGroups() const;

  static Arg no_arg;

 private:

  void Init(const string& pattern, const RE_Options* options);
  void Cleanup();

  int TryMatch(const StringPiece& text,
               int startpos,
               Anchor anchor,
               bool empty_ok,
               int *vec,
               int vecsize) const;

  bool Rewrite(string *out,
               const StringPiece& rewrite,
               const StringPiece& text,
               int *vec,
               int veclen) const;

  bool DoMatchImpl(const StringPiece& text,
                   Anchor anchor,
                   int* consumed,
                   const Arg* const args[],
                   int n,
                   int* vec,
                   int vecsize) const;

  pcre* Compile(Anchor anchor);

  string        pattern_;
  RE_Options    options_;
  pcre*         re_full_;       // For full matches
  pcre*         re_partial_;    // For partial matches
  const string* error_;         // Error indicator (or points to empty string)
};

}   // namespace pcrecpp

#endif /* _PCRECPP_H */
