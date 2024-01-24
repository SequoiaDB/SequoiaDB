
#include "system.hpp"

#ifdef linux
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <iostream>
#endif

#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <iostream>
#endif

static void ReplaceAll( string &source, string oldStr, string newStr ) ;
static void SplitString( const string& s, const string& c, vector<string>& v ) ;

BOOLEAN checkFilePath( string fullPath, string relativePath )
{
   INT32 rc = SDB_OK ;
   string newPath ;
#ifdef WIN32

   ReplaceAll( relativePath, "/", "\\" ) ;

   if ( SDB_OK == getRealPath( fullPath, newPath ) )
   {
      string de = "\\" ;
      string tmpPath = "" ;
      vector<string> list ;
      vector<string>::iterator iter ;

      SplitString( newPath, "\\", list ) ;

      for( iter = list.begin(); iter != list.end(); ++iter )
      {
         BOOLEAN exist = FALSE ;
         string dirName = *iter ;
         vector<FileStruct> fileList ;
         vector<FileStruct>::iterator iter2 ;

         if ( iter == list.begin() )
         {
            tmpPath = dirName ;
            continue ;
         }

         rc = getFiles( tmpPath, fileList ) ;
         if( rc )
         {
            return TRUE ;
         }

         for( iter2 = fileList.begin(); iter2 != fileList.end(); ++iter2 )
         {
            FileStruct info = *iter2 ;

            if ( info.fileName == dirName )
            {
               exist = TRUE ;
               break ;
            }
         }

         if ( !exist )
         {
            return FALSE ;
         }

         tmpPath = tmpPath + "\\" + dirName ;
      }

      if( newPath.find( tmpPath.c_str() ) != newPath.npos )
      {
         return TRUE ;
      }

      return FALSE ;
   }
#endif //WIN32

   return TRUE ;
}

INT32 getRealPath( string path, string& fullPath )
{
   INT32 rc = SDB_OK ;
#ifdef WIN32
   wchar_t wFileName[MAX_PATH] ;
   wchar_t buffer[MAX_PATH] ;
   CHAR tmp[MAX_PATH] ;

   MultiByteToWideChar( CP_UTF8, 0, path.c_str(), -1, wFileName, MAX_PATH ) ;

   memset( buffer, 0, MAX_PATH ) ;

   int ret = GetFullPathName( (LPCTSTR)wFileName, MAX_PATH, buffer, NULL ) ;
   if( !ret || ret > MAX_PATH )
   {
      rc = SDB_IO ;
      goto error ;
   }

   WideCharToMultiByte( CP_ACP, 0, buffer, -1, tmp, MAX_PATH, NULL, NULL ) ;

   fullPath = string( tmp ) ;

#endif //WIN32
done:
   return rc ;
error:
   goto done ;
}

INT32 getFiles( string path, vector<FileStruct> &fileList )
{
   INT32 rc = SDB_OK ;
   FileStruct fileInfo ;
#ifdef WIN32

   _finddata_t file ;
   long handle ;

   path += "\\*.*" ;

   handle = _findfirst( path.c_str(), &file ) ;
   if( handle == -1 )
   {
      rc = SDB_IO ;
      goto error ;
   }
   do
   {
      if( strcmp( file.name, "." ) == 0 || strcmp( file.name, ".." ) == 0 )
      {
         continue ;
      }
      fileInfo.isDir = ( file.attrib == _A_SUBDIR ? TRUE : FALSE ) ;
      fileInfo.fileName = file.name ;
      fileList.push_back( fileInfo ) ;
   }while( _findnext( handle, &file ) == 0 ) ;

#endif //WIN32

#ifdef linux

   DIR *pDir = NULL ;
   struct dirent *ent ;
   CHAR childpath[512] ;

   memset( childpath, 0, 512 ) ;
   pDir = opendir( path.c_str() ) ;
   if( pDir == NULL )
   {
      rc = SDB_IO ;
      goto error ;
   }

   while( ( ent = readdir( pDir ) ) != NULL )
   {
      if( strcmp( ent->d_name, "." ) == 0 || strcmp( ent->d_name, ".." ) == 0 )
      {
         continue ;
      }
      fileInfo.isDir = ( ( ent->d_type & DT_DIR ) ? TRUE : FALSE ) ;
      fileInfo.fileName = ent->d_name ;
      fileList.push_back( fileInfo ) ;
   }
   closedir( pDir ) ;

#endif //linux

done:
   return rc ;
error:
   goto done ;
}

#ifdef WIN32
INT32 utf8ToGbk( string utf8, string &gbk )
{
   INT32 rc = SDB_OK ;
   wchar_t *pGbk = NULL ;
   char *pGbk2 = NULL ;
   INT32 len = 0 ;
   
   len = MultiByteToWideChar( CP_UTF8, 0, utf8.c_str(), -1, NULL, 0 ) ;
   if( !len )
   {
      rc = SDB_IO ;
      //cout << "utf8 转 gbk 失败: " << utf8  << endl ;
      goto error ;
   }

   pGbk = new wchar_t[ len + 1 ] ;
   if( pGbk == NULL )
   {
      rc = SDB_OOM ;
      //cout << "内存不足" << endl ;
      goto error ;
   }
   memset( pGbk, 0, len + 1 ) ;

   len = MultiByteToWideChar( CP_UTF8, 0, utf8.c_str(), -1, pGbk, len ) ;
   if( !len )
   {
      rc = SDB_IO ;
      //cout << "utf8 转 gbk 失败: " << utf8  << endl ;
      goto error ;
   }

   len = WideCharToMultiByte( CP_ACP, 0, pGbk, -1, NULL, 0, NULL, NULL ) ;
   if( !len )
   {
      rc = SDB_IO ;
      goto error ;
   }
   
   pGbk2 = new char[ len + 1 ] ;
   if( pGbk2 == NULL )
   {
      rc = SDB_OOM ;
      //cout << "内存不足" << endl ;
      goto error ;
   }
   memset( pGbk2, 0, len + 1 ) ;
   len = WideCharToMultiByte( CP_ACP, 0, pGbk, -1, pGbk2, len, NULL, NULL ) ;
   if( !len )
   {
      rc = SDB_IO ;
      goto error ;
   }

   gbk = string( (char *)pGbk2 ) ;

done:
   if( pGbk )
   {
      delete pGbk ;
   }
   if( pGbk2 )
   {
      delete pGbk2    ;
   }
   return rc ;
error:
   goto done ;
}
#endif //WIN32

INT32 fileIsExist( string path )
{
   INT32 rc = SDB_OK ;
   FileStruct fileInfo ;
#ifdef WIN32

   ReplaceAll( path, "/", "\\" ) ;
   
   if( path[ path.length() - 1 ] == '\\' )
   {
      path = path.substr( 0, path.size() - 1 ) ;
   }

   _finddata_t file ;
   long handle ;
   
   string newPath ;
   
   utf8ToGbk( path, newPath ) ;

   handle = _findfirst( newPath.c_str(), &file ) ;
   if( handle == -1 )
   {
      rc = SDB_IO ;
      goto error ;
   }

#endif //WIN32

#ifdef linux

   ReplaceAll( path, "\\", "/" ) ;

   DIR *pDir = opendir( path.c_str() ) ;
   if( pDir == NULL )
   {
      if( access( path.c_str(), F_OK ) )
      {
         rc = SDB_IO ;
         goto error ;
      }
   }
   else
   {
      closedir( pDir ) ;
   }

#endif //linux

done:
   return rc ;
error:
   goto done ;
}

INT32 getLocalPath( string &path )
{
   INT32 rc = SDB_OK ;

#ifdef WIN32
   size_t len1 = MAX_PATH ;
   size_t len2 = MAX_PATH ;
   wchar_t wLocalPath[ MAX_PATH ] ;
   CHAR localPath[ MAX_PATH ] ;
   DWORD pathLength = 0 ;
   pathLength = GetModuleFileName( NULL, (LPTSTR)wLocalPath, MAX_PATH ) ;
   if( pathLength == 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   wcstombs_s( &len1, localPath, len2, wLocalPath, _TRUNCATE ) ;
   path = localPath ;
   path = path.substr( 0, path.find_last_of( '\\' ) + 1 ) ;
#endif //WIN32

#ifdef linux

   CHAR buf[1024] ;
   readlink( "/proc/self/exe", buf, 1024 ) ;
   path = buf ;
   path = path.substr( 0, path.find_last_of( '/' ) + 1 ) ;

#endif //linux

done:
   return rc ;
error:
   goto done ;
}

INT32 file_get_contents( string fileName, string &content )
{
   INT32 rc = SDB_OK ;

#ifdef WIN32
   DWORD fileSize = 0 ;
   DWORD readSize = 0 ;
   size_t len = 0 ;
   CHAR *pBuffer = NULL ;
   wchar_t wFileName[MAX_PATH] ;

   MultiByteToWideChar( CP_UTF8, 0, fileName.c_str(), -1, wFileName, MAX_PATH ) ;
   HANDLE hFile = ::CreateFile( (LPCTSTR)wFileName,
                                GENERIC_READ,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                NULL,
                                NULL ) ;
   if( hFile == INVALID_HANDLE_VALUE )
   {
      rc = SDB_IO ;
      goto error ;
   }
   fileSize = ::GetFileSize( hFile, NULL ) ;
   pBuffer = (CHAR *)malloc( fileSize + 1 ) ;
   if( pBuffer == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   pBuffer[fileSize] = 0 ;
   if( ::ReadFile( hFile, pBuffer, fileSize, &readSize, NULL ) == 0 )
   {
      CloseHandle( hFile ) ;
      free( pBuffer ) ;
      rc = SDB_IO ;
      goto error ;
   }
   CloseHandle( hFile ) ;
   content = string( pBuffer, fileSize ) ;
   free( pBuffer ) ;
   pBuffer = NULL ;

#endif //WIN32

#ifdef linux

   FILE *fp = NULL ;
   CHAR *pBuffer = NULL ;
   UINT64 fileSize = 0 ;
   struct stat statbuff ;

   if( stat( fileName.c_str(), &statbuff ) < 0 )
   {
      rc = SDB_IO ;
      goto error ;
   }
   fileSize = statbuff.st_size ;
   pBuffer = (CHAR *)malloc( fileSize + 1 ) ;
   if( pBuffer == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   pBuffer[fileSize] = 0 ;

   fp = fopen( fileName.c_str(), "r" ) ;
   if( fp == NULL )
   {
      rc = SDB_IO ;
      goto error ;
   }
   fread( pBuffer, fileSize, 1, fp ) ;
   fclose( fp ) ;
   content = string( pBuffer, fileSize ) ;
   free( pBuffer ) ;
   pBuffer = NULL ;

#endif //linux

done:
   return rc ;
error:
   goto done ;
}

INT32 file_put_contents( string fileName, string &content, BOOLEAN isAppend )
{
   INT32 rc = SDB_OK ;

#ifdef WIN32

   DWORD fileSize = 0 ;
   DWORD writeSize = 0 ;
   size_t len = 0 ;
   BOOL ret = 0 ;
   CHAR *pBuffer = NULL ;
   wchar_t wFileName[MAX_PATH] ;

   MultiByteToWideChar( CP_UTF8, 0, fileName.c_str(), -1, wFileName, MAX_PATH ) ;
   HANDLE hFile = ::CreateFile( (LPCTSTR)wFileName,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                isAppend ? OPEN_EXISTING : TRUNCATE_EXISTING,
                                NULL,
                                NULL ) ;
   if( hFile == INVALID_HANDLE_VALUE )
   {
      hFile = ::CreateFile( (LPCTSTR)wFileName,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             NULL,
                             NULL ) ;
      if( hFile == INVALID_HANDLE_VALUE )
      {
         rc = SDB_IO ;
         goto error ;
      }
   }
   else
   {
      if( isAppend )
      {
         ::SetFilePointer( hFile, 0, NULL, FILE_END ) ;
      }
   }
   if( ::WriteFile( hFile, content.c_str(), content.size(), &writeSize, NULL ) != TRUE )
   {
      CloseHandle( hFile ) ;
      rc = SDB_IO ;
      goto error ;
   }
   CloseHandle( hFile ) ;
#endif //WIN32

#ifdef linux

   FILE *fp = NULL ;
   if( isAppend )
   {
      fp = fopen( fileName.c_str(), "a+" ) ;
   }
   else
   {
      fp = fopen( fileName.c_str(), "w+" ) ;
   }
   if( fp == NULL )
   {
      rc = SDB_IO ;
      goto error ;
   }
   fwrite( content.c_str(), content.size(), 1, fp ) ;
   fclose( fp ) ;

#endif //linux

done:
   return rc ;
error:
   goto done ;
}

INT32 mkdir( string path )
{
   INT32 rc = SDB_OK ;

#ifdef WIN32

   string newPath ;

   ReplaceAll( path, "/", "\\" ) ;
   
   size_t len = 0 ;
   wchar_t wFileName[MAX_PATH] ;

   rc = fileIsExist( path ) ;
   if( rc )
   {
      //cout << "create dir, path: " << path << endl << endl ;
      MultiByteToWideChar( CP_UTF8, 0, path.c_str(), -1, wFileName, MAX_PATH ) ;
      BOOLEAN ret = CreateDirectory( wFileName, NULL ) ;
      if( ret == FALSE )
      {
         rc = SDB_IO ;
         goto error ;
      }
      rc = SDB_OK ;
   }

#endif //WIN32

#ifdef linux

   ReplaceAll( path, "\\", "/" ) ;

   rc = fileIsExist( path ) ;
   if( rc )
   {
      if( mkdir( path.c_str(), 0777 ) )
      {
         rc = SDB_IO ;
         goto error ;
      }
      rc = SDB_OK ;
   }

#endif //linux

done:
   return rc ;
error:
   goto done ;
}

void getPath( string fullPath, string &path )
{
   INT32 index = fullPath.find_last_of( '/' ) + 1 ;
   path = fullPath.substr( 0, index ) ;
}

void getFile( string path, string &file )
{
   INT32 index = path.find_last_of( '/' ) + 1 ;
   file = path.substr( index, path.size() - index ) ;
}

void getFileName( string path, string &fileName )
{
   INT32 index = path.find_last_of( '/' ) + 1 ;
   INT32 index2 = path.find_last_of( '.' ) ;
   fileName = path.substr( index, index2 - index ) ;
}

void getFileExtName( string path, string &extName )
{
   INT32 index = path.find_last_of( '.' ) + 1 ;
   extName = path.substr( index, path.size() - index ) ;
}

void ReplaceAll( string &source, string oldStr, string newStr )
{
   INT32 nPos = 0 ;
   while( ( nPos = source.find( oldStr, nPos ) ) != source.npos )
   {
      source.replace( nPos, oldStr.length(), newStr ) ;
      nPos += newStr.length() ;
   }
}

void SplitString( const string& s, const string& c, vector<string>& v )
{
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while(std::string::npos != pos2)
  {
    v.push_back(s.substr(pos1, pos2-pos1));
 
    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if(pos1 != s.length())
    v.push_back(s.substr(pos1));
}