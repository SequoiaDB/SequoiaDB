
#ifndef _PCRE_SCANNER_H
#define _PCRE_SCANNER_H

#include <assert.h>
#include <string>
#include <vector>

#include "pcrecpp.h"
#include "pcre_stringpiece.h"

namespace pcrecpp {

class PCRECPP_EXP_DEFN Scanner {
 public:
  Scanner();
  explicit Scanner(const std::string& input);
  ~Scanner();

  int LineNumber() const;

  int Offset() const;

  bool LookingAt(const RE& re) const;

  bool Consume(const RE& re,
               const Arg& arg0 = RE::no_arg,
               const Arg& arg1 = RE::no_arg,
               const Arg& arg2 = RE::no_arg
               );

  void Skip(const char* re);   // DEPRECATED; does *not* repeat
  void SetSkipExpression(const char* re);

  void DisableSkip();

  void EnableSkip();

  /***** Special wrappers around SetSkip() for some common idioms *****/

  void SkipCXXComments() {
    SetSkipExpression("\\s|//.*\n|/[*](?:\n|.)*?[*]/");
  }

  void set_save_comments(bool comments) {
    save_comments_ = comments;
  }

  bool save_comments() {
    return save_comments_;
  }

  void GetComments(int start, int end, std::vector<StringPiece> *ranges);

  void GetNextComments(std::vector<StringPiece> *ranges);

 private:
  std::string   data_;          // All the input data
  StringPiece   input_;         // Unprocessed input
  RE*           skip_;          // If non-NULL, RE for skipping input
  bool          should_skip_;   // If true, use skip_
  bool          skip_repeat_;   // If true, repeat skip_ as long as it works
  bool          save_comments_; // If true, aggregate the skip expression

  std::vector<StringPiece> *comments_;

  int           comments_offset_;

  void ConsumeSkip();
};

}   // namespace pcrecpp

#endif /* _PCRE_SCANNER_H */
