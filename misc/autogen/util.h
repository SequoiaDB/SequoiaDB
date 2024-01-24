#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <time.h>
using namespace std ;

#if defined (__linux__)
#include <string.h>
#include <linux/limits.h>
#include <sys/time.h>
#define UTIL_FILE_SEP       "/"
#define UTIL_FILE_SEP_CHAR  '/'
#define UTIL_MAX_PATHSIZE   PATH_MAX
#elif defined (_WIN32)
#define UTIL_FILE_SEP       "\\"
#define UTIL_FILE_SEP_CHAR  '\\'
#define UTIL_MAX_PATHSIZE   _MAX_PATH
#endif

#if defined (_WIN32)

#include <Winsock2.h> // timeval

   #if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
      #define DELTA_EPOCH_IN_MICROSECS  11644473600000000
   #else
      #define DELTA_EPOCH_IN_MICROSECS  11644473600000000LL
   #endif //(_MSC_VER) || defined(_MSC_EXTENSIONS)

struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday( struct timeval *tv, struct timezone *tz ) ;
#endif

#define PD_SERVER    0
#define PD_ERROR     1
#define PD_WARNING   2
#define PD_INFO      3
#define printLog( level ) if ( getCmdOptions()->getLevel() >= level ) cout

size_t utilSnprintf( char* pBuffer, int iLength, const char* pFormat, ... ) ;

vector<string> stringSplit( string s, string seperator ) ;

string& utilReplaceAll( string &str, const string& old_value, const string &new_value ) ;

bool utilStrStartsWith( const string& str, const string& substr ) ;

int utilGetMaxInt( int a, int b ) ;

int utilGetEWD ( char *pBuffer, int maxlen ) ;
int utilGetEWD2( string &path ) ;

int utilGetCWD ( char *pPath, int maxSize ) ;
int utilGetCWD2( string &path ) ;

string utilCatPath ( const char *path1, const char *path2 ) ;
string utilCatPath2( const string &path1, const string &path2 ) ;

string utilGetRealPath2( const char *pPath ) ;
char* utilGetRealPath ( const char  *pPath,
                        char *resolvedPath,
                        int length ) ;

int utilGetCurrentYear() ;

string utilGetSuffixByFileName( string fileName ) ;

void utilGetFileNameListBySuffix( const char *path,
                                  const char *suffix[], int suffixSize,
                                  const char *skipDir[], int skipDirSize,
                                  vector<string> &files ) ;

void utilGetPathListBySuffix( const char *path,
                              const char *suffix[], int suffixSize,
                              const char *skipDir[], int skipDirSize,
                              vector<string> &files ) ;

void utilGetFileListBySuffix( const char *path,
                              const char *suffix[], int suffixSize,
                              const char *skipDir[], int skipDirSize,
                              bool isFullPath,
                              vector<string> &files ) ;

int utilStrncasecmp( const char *pString1, const char *pString2,
                     size_t iLength ) ;

unsigned int utilHashFileName( const char *fileName ) ;

unsigned int utilHash( const char *data, int len ) ;

int utilStrToBool( const char* pString, bool* pBoolean ) ;

int utilGetFileContent( const char* path, string &content ) ;

int utilPutFileContent( const char *path, const char *content ) ;

bool utilPathExist( const char *path ) ;

#endif