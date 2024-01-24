#if defined (_WIN32)
#include <windows.h>
#endif
#include "table.hpp"
#include "string.h"
#include <stdint.h>

#define PERIOD_CH          "\u3002"
#define COMMA_CH           "\uFF0C"
#define PERIOD_EN          "."
#define COMMA_EN           ","
#define BACKSLASH          "\\"
#define TABLE_LINE_LEN     24
#define HTML_TAG_BR        "<br>"

#if defined (_WIN32)
string _gbk2utf8(const string &input)
{
    string output;
    WCHAR *str1 = NULL;
    int n = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, NULL, 0);
    str1 = new WCHAR[n];
    MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, str1, n);
    n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
    char *str2 = new char[n];
    WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
    output = str2;
    delete[] str1;
    str1 = NULL;
    delete[] str2;
    str2 = NULL;
    return output;
}
#endif

string& _trim(string &s)
{
    if (s.empty()) return s;
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

string _trim_element_space(string &src_str)
{
    unsigned int i = 0, beg = 0, dist = 0;
    const char *text = src_str.c_str();
    const char *txt_pos  = NULL;
    const char *txt_mark = NULL;
    unsigned int len = strlen(text);
    vector<int> vec_pos;
    string retStr;

    // get the positions of '|'
    while(beg <= len)
    {
        txt_pos = text + beg;
        txt_mark = strchr(txt_pos, '|');
        if (txt_mark == NULL)
        {
            break;
        }
        else
        {
            if (txt_mark == text)
            {
                dist = 0;
            }
            else if ('\\' == *(txt_mark - 1))
            {
                while (txt_mark - text < len &&
                       NULL != (txt_mark = strchr(txt_mark + 1, '|')) &&
                       '\\' == *(txt_mark - 1))
                {
                    ; // do nothing
                }
                if (NULL == txt_mark ||
                    '\\' == *(txt_mark - 1) ||
                    (txt_mark - text) >= len)
                {
                    break;
                }
                dist = txt_mark - text;
            }
            else
            {
                dist = txt_mark - text;
            }
            vec_pos.push_back(dist);
            beg = dist + 1;
        }
    } // while

    // trim space
    if (vec_pos.size() < 2)
    {
        return retStr;
    }
    retStr = "|";
    for(i = 0; i < vec_pos.size() - 1; i++)
    {
        string tmpStr = 
            string(src_str, vec_pos[i] + 1, vec_pos[i+1] - vec_pos[i] - 1);
        tmpStr = _trim(tmpStr);
        retStr += tmpStr + "|";
    }
    return retStr;
}

void _get_elems(const string &data, vector<string> &vec_out)
{
    const uint8_t *text = (const uint8_t *)data.c_str();
    const uint8_t *txt_pos = NULL;
    size_t len = data.length();
    size_t i, beg = 0; 
    size_t dist = 0;
    vector<size_t> vec_idx;
    
    while(beg <= len)
    {
        txt_pos = text + beg;
        txt_pos = (const uint8_t *)strchr((const char *)txt_pos, '|');
        if (NULL == txt_pos)
        {
            break;
        }
        else if (txt_pos == text)
        {
            dist = 0;
        }
        else if ('\\' != *(txt_pos - 1))
        {
            dist = txt_pos - text;
        }
        else
        {
            dist = txt_pos - text;
            beg = dist + 1;
            continue;
        }
        vec_idx.push_back(dist);
        beg = dist + 1;
    }

    if (vec_idx.size() < 2) return;

    for(i = 0; i < vec_idx.size() - 1; i++)
    {
        string tmp = 
            string(data, vec_idx[i] + 1, vec_idx[i+1] - vec_idx[i] - 1);
        // TODO:
        // tmp = _filter_chars(tmp);
        vec_out.push_back(_trim(tmp));
    }
}

size_t _utf8_to_charset(const string &input, vector<string> &output)
{
   string ch;
   size_t i = 0, len = 0;
   for (; i != input.length(); i += len)
   {
     unsigned char byte = (unsigned)input[i];
     if (byte >= 0xFC) // lenght 6
       len = 6;
     else if (byte >= 0xF8)
       len = 5;
     else if (byte >= 0xF0)
       len = 4;
     else if (byte >= 0xE0)
       len = 3;
     else if (byte >= 0xC0)
       len = 2;
     else
       len = 1;
     ch = input.substr(i, len);
     output.push_back(ch);
   }
   return output.size();
}

size_t _char_to_word(const string &text, vector<string> &output)
{
    size_t word_len = 0;
    vector<string> vec_chars;
    vector<string>::iterator it;
    string word;
    string digit;
    
    _utf8_to_charset(text, vec_chars);
    for(it = vec_chars.begin(); it != vec_chars.end(); it++)
    {
        word_len = it->length();
        if (word_len == 1)
        {
            if ((*it >= "a" && *it <= "z") ||
                (*it >= "A" && *it <= "Z"))
            {
                if (!digit.empty())
                {
                    output.push_back(digit);
                    digit.clear();
                }
                word += *it;
            }
            else if (*it == "+" || *it == "-" || (*it >= "0" && *it <= "9"))
            {
                if (!word.empty())
                {
                    output.push_back(word);
                    word.clear();
                }
                digit += *it;
            }
            else
            {
                // save the word and digits which had been cached first
                if (!word.empty())
                {
                    output.push_back(word);
                    word.clear();
                }
                if (!digit.empty()) 
                {
                    output.push_back(digit);
                    digit.clear();
                }
                // and then save the current charactor
                output.push_back(*it);
            }
        }
        else if (word_len > 1)
        {
            // keep the word and digits which had been cached first
            if (!word.empty())
            {
                output.push_back(word);
                word.clear();
            }
            if (!digit.empty()) 
            {
                output.push_back(digit);
                digit.clear();
            }
            // and then keep the current charactor
            output.push_back(*it);
        }
    } // for
    // keep the word and digits we had cached
    if (!word.empty())
    {
        output.push_back(word);
        word.clear();
    }
    if (!digit.empty()) 
    {
        output.push_back(digit);
        digit.clear();
    }
    return output.size();
}

void _join_tag(const vector<string> &input, vector<string> &output, const string &tag)
{
    vector<string>::const_iterator it ;
    string temp ;

    // e.g: tag = "<br>" ==> vec_tag = [<,br,>]
    vector<string> vec_tag ;
    _char_to_word( tag, vec_tag ) ;

    // it traverses each word in input
    for ( it = input.begin(); it != input.end(); it++ )
    {
        // if *it matches "<"
        int tag_index = 0 ;
        if( *it == vec_tag[ tag_index ] )
        {
            // inside match_it traverses and matches each word in matches.
            vector<string>::const_iterator match_it = it ;
            for( ; tag_index < vec_tag.size() &&
                   match_it != input.end() &&
                   *match_it == vec_tag[ tag_index ]; tag_index++, match_it++ )
            {
                temp += *match_it ;
            }

            // matched
            if( tag_index >= vec_tag.size() )
            {
                output.push_back( temp ) ;
                /* remove the add of it++,
                so it++ in next input loop can points to next word.*/
                it = --match_it ;
            }
            else
            {
                // don't matched
                output.push_back( *it ) ;
            }

            temp.clear() ;
            continue ;
        }

        // if *it does not match "<"
        output.push_back( *it ) ;
    }
}

void _split_elem(const string &text, vector<string> &vec_out)
{
    string one_line;
    string pre_char;
    vector<string> vec_words;
    vector<string> vec_words_and_tags;
    vector<string>::iterator it;
    int left = TABLE_LINE_LEN;

    // get words, the output is: "abc", " ", ",", "1", "1024", "集合",...
    _char_to_word(text, vec_words);

    // join words to tag, e.g: join "<","br",">" to "<br>"
    string tag = HTML_TAG_BR ;
    _join_tag( vec_words, vec_words_and_tags, tag ) ;

    // build line
    for(it = vec_words_and_tags.begin(); it != vec_words_and_tags.end(); it++)
    {
        left -= it->length();
        if (left > 0)
        {
            one_line += *it;
            pre_char = *it;
        }
        else
        {
            int has_handle = 0;
            // if it's "," and ".", we still let 
            // it put to the end of the line
            if (*it == COMMA_EN ||
                *it == PERIOD_EN ||
                pre_char == BACKSLASH ||
#if defined (_WIN32)
                *it == _gbk2utf8(COMMA_CH) ||
                *it == _gbk2utf8(PERIOD_CH))
#else
                *it == COMMA_CH ||
                *it == PERIOD_CH )
#endif
            {
                one_line += *it;
                has_handle = 1;
            }
            // put the current line into vector
            if (!one_line.empty())
            {
                vec_out.push_back(one_line);
            }
            // clean up
            one_line.clear();
            pre_char = "";
            // try to start a new line
            if (!has_handle)
            {
                // start a new line
                one_line += *it;
                pre_char = *it;
                left = TABLE_LINE_LEN - it->length();
            }
            else
            {
                left = TABLE_LINE_LEN;
            }
        }
    }
    // put the last line to the output
    if (!one_line.empty())
    {
        vec_out.push_back(one_line);
    }
}

void _rebuild_table(const string &text, vector<string> &vec_out)
{
    size_t line_num = 0, column_num = 0;
    size_t i = 0;
    vector<string> vec_elems;
    vector< vector<string> > vvec_elems;
    vector<string>::iterator vit;
    vector< vector<string> >::iterator vvit;

    // get elements
    _get_elems(text, vec_elems);
    column_num = vec_elems.size();
    
    // split elements
    for(vit = vec_elems.begin(); vit != vec_elems.end(); vit++)
    {
        vector<string> vec_tmp;
        _split_elem(*vit, vec_tmp);
        vvec_elems.push_back(vec_tmp);
        if (vec_tmp.size() > line_num) line_num = vec_tmp.size();
    }
    
    // combine the elements
    for(i = 0; i < line_num; i++)
    {
        string line;
        line += "|";
        for(vvit = vvec_elems.begin(); vvit != vvec_elems.end(); vvit++)
        {
            vector<string> vec_tmp = *vvit;
            if (vec_tmp.size() > i)
            {
                line += vec_tmp[i];
            }
            line += "|";
        }
        vec_out.push_back(line);
    }
}

void convert_table(string &text, vector<string> &vec_out)
{
    // trim space
    text = _trim(text);
    // rebuild table
    _rebuild_table(text, vec_out);
}
