#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

bool is_dir ( const char *path )
{
   struct stat statbuf ;
   if ( lstat( path, &statbuf ) == 0 )
   {
      return S_ISDIR ( statbuf.st_mode) != 0 ;
   }
   return false ;
}
bool is_file ( const char *path )
{
   struct stat statbuf ;
   if ( lstat ( path, &statbuf ) == 0 )
   {
      return S_ISREG ( statbuf.st_mode ) != 0 ;
   }
   return false ;
}
bool is_special_dir ( const char *path )
{
   return strcmp ( path,".") == 0 || strcmp ( path, ".." ) == 0 ;
}
void get_file_path ( const char *path, const char *file_name, char *file_path )
{
   strcpy ( file_path, path ) ;
   if ( file_path[strlen(path) - 1] != '/' )
      strcat ( file_path, "/" ) ;
   strcat ( file_path, file_name ) ;
}

void deleteFile ( const char *path )
{
   DIR *dir ;
   dirent *dir_info ;
   char file_path[PATH_MAX] ;
   if ( is_file( path ) )
   {
      remove ( path ) ;
      return ;
   }
   if ( is_dir( path ) )
   {
      if ( ( dir = opendir(path) ) == NULL )
      {
         return ;
      }
      while ( ( dir_info = readdir(dir) ) != NULL)
      {
         get_file_path ( path, dir_info->d_name, file_path ) ;
         if ( is_special_dir( dir_info->d_name ) )
            continue ;
         deleteFile ( file_path ) ;
         rmdir ( file_path ) ;
      }
   }
}
/*
int main( int argc, char **argv )
{
   delete_file ( "/home/users/tanzhaobo/abcd" ) ;
   return 0 ;
}
*/
