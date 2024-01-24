#ifndef _DELETE_FILE
#define _DELETE_FILE

bool is_dir ( const char *path ) ;
bool is_file ( const char *path ) ;
bool is_special_dir ( const char *path ) ;
void get_file_path ( const char *path,
                     const char *file_name,
                     char *file_path ) ;
void deleteFile ( const char *path ) ;

#endif
