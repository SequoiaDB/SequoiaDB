
#ifndef SYSTEM_HPP__
#define SYSTEM_HPP__

#include <vector>
#include <string>
#include "core.h"
using namespace std ;

typedef struct
{
   BOOLEAN isDir ;
   string fileName ;
} FileStruct ;

BOOLEAN checkFilePath( string fullPath, string relativePath ) ;
INT32 getRealPath( string path, string& fullPath ) ;
INT32 getFiles( string path, vector<FileStruct> &fileList ) ;
INT32 getLocalPath( string &path ) ;
INT32 fileIsExist( string path ) ;
INT32 file_get_contents( string fileName, string &content ) ;
INT32 file_put_contents( string fileName,
                         string &content,
                         BOOLEAN isAppend = FALSE ) ;
INT32 mkdir( string path ) ;

void getPath( string fullPath, string &path ) ;
void getFile( string path, string &file ) ;
void getFileName( string path, string &fileName ) ;
void getFileExtName( string path, string &extName ) ;

#endif