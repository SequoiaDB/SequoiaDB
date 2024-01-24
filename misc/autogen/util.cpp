#include "util.h"
#include <stdarg.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
namespace fs = boost::filesystem ;


#if defined (_WIN32)
   #include <Winsock2.h>
   #include <Windows.h>
   #include <direct.h>

   #define _utilGetCWD  _getcwd
#endif

#if defined (__linux__)
   #include <unistd.h>
   #define  _utilGetCWD getcwd

   #define PROC_SELF_EXE                  "/proc/self/exe"

#endif

#if defined (_WIN32)
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
   FILETIME ft;
   UINT64 tmpres = 0;
   static int tzflag;

   if (NULL != tv)
   {
      GetSystemTimeAsFileTime(&ft);

      tmpres |= ft.dwHighDateTime;
      tmpres <<= 32;
      tmpres |= ft.dwLowDateTime;

      /*convert into microseconds*/
      tmpres /= 10;

      /*converting file time to unix epoch*/
      tmpres -= DELTA_EPOCH_IN_MICROSECS;

      tv->tv_sec = (long)(tmpres / 1000000UL);
      tv->tv_usec = (long)(tmpres % 1000000UL);
   }

   if (NULL != tz)
   {
      long ttz;
      int tdl;
      if (!tzflag)
      {
         _tzset();
         tzflag++;
      }
      _get_timezone(&ttz);
      _get_daylight(&tdl);
      tz->tz_minuteswest = ttz / 60;
      tz->tz_dsttime = tdl;
   }

   return 0;
}

#endif

size_t utilSnprintf(char* pBuffer, int iLength, const char* pFormat, ...)
{
   va_list ap;
   size_t n;
   va_start(ap, pFormat);
#if defined (_WIN32)
   n=_vsnprintf_s(pBuffer, iLength, _TRUNCATE, pFormat, ap);
#else
   n=vsnprintf(pBuffer, iLength, pFormat, ap);
#endif
   va_end(ap);
   // Set terminate if the length is greater than buffer size
   if((n<0) || n>=(size_t)iLength)
      n=iLength-1;
   pBuffer[n]='\0';
   return n;
}

typedef string::size_type string_size ;
vector<string> stringSplit( string s, string seperator )
{
   vector<string> result ;
   string_size i = 0;

   while(i != s.size())
   {
      int flag = 0;
      while(i != s.size() && flag == 0)
      {
         flag = 1;
         for(string_size x = 0; x < seperator.size(); ++x)
         {
            if(s[i] == seperator[x])
            {
               ++i;
               flag = 0;
               break;
            }
         }
      }

      flag = 0;
      string_size j = i ;
      while(j != s.size() && flag == 0)
      {
         for(string_size x = 0; x < seperator.size(); ++x)
         {
            if(s[j] == seperator[x])
            {
               flag = 1;
               break;
            }
         }

         if( flag == 0 )
         {
            ++j ;
         }
      }

      if(i != j)
      {
         result.push_back(s.substr(i, j-i));
         i = j;
      }
   }

   return result;
}

string& utilReplaceAll( string &str, const string& old_value, const string &new_value )
{

   for ( string::size_type pos(0); pos != string::npos; pos += new_value.length() )
   {
      pos = str.find ( old_value, pos ) ;
      if ( string::npos == pos )
      {
         break ;
      }

      str.replace( pos, old_value.length(), new_value ) ;
   }

   return str ;
}

bool utilStrStartsWith( const string& str, const string& substr )
{
   if ( str.empty() || substr.empty() )
   {
      return false ;
   }

   return str.compare( 0, substr.size(), substr ) == 0 ;
}

int utilGetMaxInt( int a, int b )
{
   return ( a >= b ? a : b ) ;
}

#if defined (_WIN32)
int _wc2ANSI ( LPCWSTR lpcszWCString,
              LPSTR   *plppszString,
              DWORD   *plpdwString )
{
   int rc           = 0 ;
   int strSize      = 0 ;
   int requiredSize = 0 ;
   requiredSize       = WideCharToMultiByte ( CP_ACP, 0, lpcszWCString,
                                              -1, NULL, 0, NULL, NULL ) ;
   // caller is responsible to free memory
   *plppszString = (LPSTR)malloc ( requiredSize ) ;
   if ( !plppszString )
   {
      rc = 1 ;
      goto error ;
   }
   strSize = WideCharToMultiByte ( CP_ACP, 0, lpcszWCString, -1,
                                   *plppszString, requiredSize,
                                   NULL, NULL ) ;
   if ( 0 == strSize )
   {
      free ( *plppszString ) ;
      rc = 1 ;
      *plppszString = NULL ;
      goto error ;
   }

   if ( plpdwString )
   {
      *plpdwString = (DWORD)strSize ;
   }

done:
   return rc ;
error:
   goto done ;
}
#endif

int utilGetEWD2( string &path )
{
   int rc = 0 ;
   char pPath[UTIL_MAX_PATHSIZE + 1] ;

   memset( pPath, 0, UTIL_MAX_PATHSIZE + 1 ) ;
   rc = utilGetEWD( pPath, UTIL_MAX_PATHSIZE ) ;
   if ( rc )
   {
      goto error ;
   }

   path = pPath ;

done:
   return rc ;
error:
   goto done ;
}

int utilGetEWD( char *pBuffer, int maxlen )
{
   int rc = 0 ;
#if defined (__linux__)
   char *tSep = NULL ;
   char sep = '/' ;
   int len = readlink(PROC_SELF_EXE, pBuffer, maxlen ) ;
   if ( len <= 0 || len >= maxlen )
   {
      rc = 1 ;
      goto error ;
   }
   pBuffer[len] = '\0' ;
   tSep = strrchr ( pBuffer, sep ) ;
   if ( tSep )
      *tSep = '\0' ;
#elif defined (_WIN32)
   LPSTR lpszPath = NULL ;
   WCHAR *tSep = NULL ;
   WCHAR sep = '\\' ;
   WCHAR lpszwPath[UTIL_MAX_PATHSIZE + 1] = {0} ;
   if ( !GetModuleFileName ( NULL, lpszwPath, UTIL_MAX_PATHSIZE ) )
   {
      rc = 1 ;
      goto error ;
   }
   tSep = wcsrchr ( lpszwPath, sep ) ;
   // path with prefix "?\"
   if ( tSep )
      *tSep = '\0' ;
   // path not with prefix
   else
   {
      lpszwPath[0] = '.' ;
      lpszwPath[1] = '\0' ;
   }
   // lpszPath is free at the end of this function 
   rc = _wc2ANSI ( lpszwPath, &lpszPath, NULL ) ;
   if ( rc )
   {
      goto error ;
   }
   size_t len = strlen ( lpszPath ) ;
   if ( len > maxlen )
   {
      rc = 1 ;
      goto error ;
   }
   strncpy ( pBuffer, lpszPath, len +1 ) ;
#endif
done:
#if defined (_WIN32)
   if ( lpszPath )
   {
      free( lpszPath ) ;
   }
#endif
   return rc ;
error:
   goto done ;
}

int utilGetCWD2( string &path )
{
   int rc = 0 ;
   char pPath[UTIL_MAX_PATHSIZE + 1] ;

   memset( pPath, 0, UTIL_MAX_PATHSIZE + 1 ) ;
   rc = utilGetCWD( pPath, UTIL_MAX_PATHSIZE ) ;
   if ( rc )
   {
      goto error ;
   }

   path = pPath ;

done:
   return rc ;
error:
   goto done ;
}

int utilGetCWD( char *pPath, int maxSize )
{
   int rc = 0 ;

   if ( NULL == pPath || 0 == maxSize )
   {
      rc = 1 ;
   }
   else
   {
      if ( NULL == _utilGetCWD( pPath, maxSize ) )
      {
         int err = errno ;

         switch ( err )
         {
         case EINVAL:
         case ERANGE:
            rc = 1 ;
            break ;
         case EACCES:
         case EPERM:
            rc = 1 ;
            break ;
         default:
            rc = 1 ;
            break ;
         }
      }
   }

   return rc ;
}

string utilCatPath( const char *path1, const char *path2 )
{
   string tmp1 = path1 ;
   string tmp2 = path2 ;

   return utilCatPath2( tmp1, tmp2 ) ;
}

string utilCatPath2( const string &path1, const string &path2 )
{
   string newPath ;
   const char *ptr = path1.c_str() ;
   size_t len = path1.length() ;

   newPath = path1 ;

   if ( len > 0 && UTIL_FILE_SEP_CHAR != ptr[ len - 1 ] &&
        ( path2.empty() || path2.at( 0 ) != UTIL_FILE_SEP_CHAR ) )
   {
      newPath += UTIL_FILE_SEP ;
   }

   newPath += path2 ;

   return newPath ;
}

string utilGetRealPath2( const char *pPath )
{
   string realPath ;
   char tmp[UTIL_MAX_PATHSIZE + 1] ;

   memset( tmp, 0, UTIL_MAX_PATHSIZE + 1 ) ;
   if( utilGetRealPath( pPath, tmp, UTIL_MAX_PATHSIZE ) )
   {
      realPath = tmp ;
   }

   return realPath ;
}

char* utilGetRealPath( const char  *pPath,
                       char        *resolvedPath,
                       int          length )
{
   // sanity check, only take effect in debug build
   char pathBuffer [ UTIL_MAX_PATHSIZE + 1 ] = {'\0'} ;
   char tempBuffer [ UTIL_MAX_PATHSIZE + 1 ] = {'\0'} ;
   strncpy ( pathBuffer, pPath, sizeof(pathBuffer) ) ;
   char *ret = NULL ;
   char *pPos = NULL ;

   resolvedPath[ 0 ] = '\0' ;

   while ( 1 )
   {
      // if we had changed the first / to '\0',
      // that means the input path(assume it is "/a/b/c") does not exist,
      // and 'realpath' can not deal with this situation in linux.
      // so let's return "\0a/b/c" out of the loop
      if ( '\0' == pathBuffer[0] )
      {
         ret = pathBuffer ;
         break ;
      }
      // try to get real path
      ret =
#if defined (__linux__)
         realpath ( pathBuffer, tempBuffer ) ;
#elif defined (_WIN32)
         _fullpath ( tempBuffer, pathBuffer, sizeof(tempBuffer) ) ;
#endif
      // if we are able to build real path, let's just return
      if ( ret )
      {
         strncpy ( resolvedPath, tempBuffer, length ) ;
         break ;
      }
      // search from right for the next /
      char *pNewPos = strrchr ( pathBuffer, UTIL_FILE_SEP_CHAR ) ;
      // if we can find /, let's set it to '\0' and continue get realpath
      if ( pNewPos )
      {
         *pNewPos = '\0' ;
      }
      else
      {
         // if we cannot find any /, and we still not able to build real path,
         // that means the input relative path is like "a/b/c", let's replenish
         // the first path of absolute path, and then replenish the rest
         strncpy( resolvedPath, tempBuffer, length ) ;
         ret = pathBuffer ;
         break ;
      }
      // when we get here that means we find the previous /
      if ( pPos )
      {
         // if we have previously set / to '\0', let's restore the /
         *pPos = UTIL_FILE_SEP_CHAR ;
      }
      pPos = pNewPos ;
   }
   if ( ret && pPos )
   {
      *pPos = UTIL_FILE_SEP_CHAR ;
      strncat ( resolvedPath, pPos, length - strlen(resolvedPath) ) ;
   }
   return ret ? resolvedPath: NULL ;
}

int utilGetCurrentYear()
{
   struct tm otm ;
   struct timeval tv;
   struct timezone tz;
   time_t tt ;

   gettimeofday( &tv, &tz ) ;

   tt = tv.tv_sec ;

#if defined (_WIN32)
   localtime_s( &otm, &tt ) ;
#else
   localtime_r( &tt, &otm ) ;
#endif

   return otm.tm_year + 1900 ;
}

string utilGetSuffixByFileName( string fileName )
{
   return fileName.substr( fileName.find_last_of( '.' ) + 1 ) ;
}

static int _isFileBySuffix( const string &fileName, const char *suffix[],
                            int size )
{
   int rc = 0 ;

   string fileSuffix = utilGetSuffixByFileName( fileName ) ;

   for ( int i = 0 ; i < size; ++i )
   {
      if( fileSuffix.length() == strlen( suffix[i] ) &&
          utilStrncasecmp( fileSuffix.c_str(), suffix[i],
                           fileSuffix.length() ) == 0 )
      {
         rc = 1 ;
         break ;
      }
   }

   return rc ;
}

int isFilterDir( const string &dirName, const char *skipDir[], int size )
{
   int rc = 0 ;

   for ( int i = 0 ; i < size; ++i )
   {
      if ( dirName.length() == strlen( skipDir[i] ) &&
           utilStrncasecmp( dirName.c_str(), skipDir[i], dirName.length() ) == 0 )
      {
         rc = 1 ;
         break ;
      }
   }

   return rc ;
}

void utilGetFileNameListBySuffix( const char *path,
                                  const char *suffix[], int suffixSize,
                                  const char *skipDir[], int skipDirSize,
                                  vector<string> &files )
{
   utilGetFileListBySuffix( path, suffix, suffixSize, skipDir, skipDirSize,
                            false, files ) ;
}

void utilGetPathListBySuffix( const char *path,
                              const char *suffix[], int suffixSize,
                              const char *skipDir[], int skipDirSize,
                              vector<string> &files )
{
   utilGetFileListBySuffix( path, suffix, suffixSize, skipDir, skipDirSize,
                            true, files ) ;
}

void utilGetFileListBySuffix( const char *path,
                              const char *suffix[], int suffixSize,
                              const char *skipDir[], int skipDirSize,
                              bool isFullPath,
                              vector<string> &files )
{
   fs::path pathFS( path ) ;
   fs::directory_iterator end_iter ;

   if ( !fs::exists( pathFS ) || !fs::is_directory( pathFS ) )
   {
      goto done ;
   }

   for ( fs::directory_iterator dir_iter( pathFS );
         dir_iter != end_iter; ++dir_iter )
   {
      if ( fs::is_regular_file( dir_iter->status() ) )
      {
         string fileName = dir_iter->path().filename().string() ;

         if ( _isFileBySuffix ( fileName, suffix, suffixSize ) )
         {
            if ( isFullPath )
            {
               fileName = utilCatPath( path, fileName.c_str() ) ;
            }

            files.push_back( fileName ) ;
         }
      }
      else if ( fs::is_directory ( dir_iter->status() ) )
      {
         if( !isFilterDir( dir_iter->path().filename().string(),
                           skipDir, skipDirSize ) )
         {
            utilGetFileListBySuffix( dir_iter->path().string().c_str(),
                                     suffix, suffixSize,
                                     skipDir, skipDirSize,
                                     isFullPath, files ) ;
         }
      }
   }

done:
   return ;
}

int utilStrncasecmp( const char *pString1, const char *pString2,
                     size_t iLength )
{
#if defined (__linux__)
   return strncasecmp(pString1, pString2, iLength);
#else
   if (iLength==0)
      return 0;
   while (iLength--!=0 && tolower(*pString1)==tolower(*pString2))
   {
      if(iLength==0||*pString1=='\0'||*pString2=='\0')
         break;
      pString1++;
      pString2++;
   }
   return tolower(*(unsigned char*)pString1) - tolower(*(unsigned
                    char*)pString2);
#endif
}

unsigned int utilHashFileName( const char *fileName )
{
   const char *pathSep = UTIL_FILE_SEP ;
   const char *pFileName = strrchr ( fileName, pathSep[0] ) ;

   if ( !pFileName )
      pFileName = fileName ;
   else
      pFileName++ ;

   return utilHash( pFileName, (int)strlen( pFileName ) ) ;
}

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const UINT16 *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((unsigned int)(((const unsigned char *)(d))[1])) << 8)\
                       +(unsigned int)(((const unsigned char *)(d))[0]) )
#endif
unsigned int utilHash( const char *data, int len )
{
   unsigned int hash = len, tmp ;
   int rem ;
   if ( len <= 0 || data == NULL ) return 0 ;
   rem = len&3 ;
   len >>= 2 ;
   for (; len > 0 ; --len )
   {
      hash += get16bits (data) ;
      tmp   = (get16bits (data+2) << 11) ^ hash;
      hash  = (hash<<16)^tmp ;
      data += 2*sizeof(unsigned short) ;
      hash += hash>>11 ;
   }
   switch ( rem )
   {
   case 3:
      hash += get16bits (data) ;
      hash ^= hash<<16 ;
      hash ^= ((signed char)data[sizeof (unsigned short)])<<18 ;
      hash += hash>>11 ;
      break ;
   case 2:
      hash += get16bits(data) ;
      hash ^= hash <<11 ;
      hash += hash >>17 ;
      break ;
   case 1:
      hash += (signed char)*data ;
      hash ^= hash<<10 ;
      hash += hash>>1 ;
   }
   hash ^= hash<<3 ;
   hash += hash>>5 ;
   hash ^= hash<<4 ;
   hash += hash>>17 ;
   hash ^= hash<<25 ;
   hash += hash>>6 ;
   return hash ;
}

const char *OSSTRUELIST[]={
   "YES",
   "yes",
   "Y",
   "y",
   "TRUE",
   "true",
   "T",
   "t",
   "1"};

// All strings represent "false"
const char *OSSFALSELIST[]={
   "NO",
   "no",
   "N",
   "n",
   "FALSE",
   "false",
   "F",
   "f",
   "0"};

int utilStrToBool( const char* pString, bool* pBoolean )
{
   int  rc           = 0 ;
   unsigned int i    = 0 ;
   size_t len        = strlen(pString) ;

   if (0 == len)
   {
      *pBoolean=false ;
      rc = 1 ;
      goto error ;
   }
   for(; i < sizeof(OSSTRUELIST)/sizeof(const char *); i++)
   {
      if(utilStrncasecmp(pString, OSSTRUELIST[i], len) == 0)
      {
         *pBoolean = true ;
         goto done ;
      }
   }
   for(i = 0; i < sizeof(OSSFALSELIST)/sizeof(const char *); i++)
   {
      if(utilStrncasecmp(pString, OSSFALSELIST[i], len) == 0)
      {
         *pBoolean = false ;
         goto done ;
      }
   }

   *pBoolean = false ;
   rc = 1 ;
   goto error ;

done :
   return rc ;
error :
   goto done ;
}

int utilGetFileContent( const char* path, string &content )
{
   int rc = 0 ;
   stringstream buffer ;
   ifstream fin( path, ios::binary);

   if( !fin )
   {
      rc = 1 ;
      goto error ;
   }

   buffer << fin.rdbuf();
   content = buffer.str() ;
   fin.close() ;

done:
   return rc ;
error:
   goto done ;
}

int utilPutFileContent( const char *path, const char *content )
{
   int rc = 0 ;
   ofstream fout( path, ios::binary);

   if ( !fout )
   {
      rc = 1 ;
      goto error ;
   }

   fout << content ;
   fout.close() ;

done:
   return rc ;
error:
   goto done ;
}

bool utilPathExist( const char *path )
{
   boost::filesystem::path path_file( path ) ;

   return boost::filesystem::exists( path_file ) ;
}