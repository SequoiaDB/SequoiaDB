#ifndef MANDOC_COMMON_H
#define MANDOC_COMMON_H
#if defined ( _WIN32 )
#include <stdarg.h>
//#define wchar_t __wchar_t
//size_t snprintf(char* pBuffer, size_t iLength, const char* pFormat, ...) ;
size_t asprintf ( char **ret, const char *format, ... ) ;
int wcwidth(wchar_t ucs) ;
//int strcasecmp ( const char *pString1, const char *pString2 );
int isblank ( char c );
#endif // #if defined _WIN32
#endif