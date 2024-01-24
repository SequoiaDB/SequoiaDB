#ifndef TABLE_HPP__
#define TABLE_HPP__

#include <stdio.h>
#include <vector>
#include <string>
#ifdef _WIN32
#include <stdint.h>
#endif

using std::vector;
using std::string;

void convert_table(string &text, vector<string> &vec_out);

#endif // TABLE_HPP__
