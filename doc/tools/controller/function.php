<?php

function getOSInfo()
{
   return (DIRECTORY_SEPARATOR == '\\') ? 'windows' : 'linux' ;
}

function removeDir( $path )
{
   if( is_dir( $path ) == false )
   {
      return false ;
   }
   $handle = @opendir( $path ) ;
   while( ( $file = @readdir( $handle ) ) != false )
   {
      if( $file != '.' && $file != '..' )
      {
         $newPath = getOSInfo() == 'windows' ? ( $path.'\\'.$file ) : ( $path.'/'.$file ) ;
         is_dir( $newPath ) ? removeDir( $newPath ) : @unlink( $newPath ) ;
      }
   }
   closedir( $handle ) ;
   return rmdir( $path ) ;
}

//格式化doxygen，因为ie必须没有dom和必须\r\n换行才能正常使用（嵌套的时候）
function formatApiDoc( $path )
{
   if( is_dir( $path ) == false )
   {
      return false ;
   }
   $handle = @opendir( $path ) ;
   while( ( $file = @readdir( $handle ) ) != false )
   {
      if( $file != '.' && $file != '..' )
      {
         $newPath = getOSInfo() == 'windows' ? ( $path.'\\'.$file ) : ( $path.'/'.$file ) ;
         if( is_dir( $newPath ) )
         {
            if( formatApiDoc( $newPath ) === false )
            {
               return false ;
            }
         }
         else
         {
            $pathInfo = pathinfo( $file ) ;

            if( array_key_exists( 'extension', $pathInfo ) && ( $pathInfo['extension'] == 'html' || $pathInfo['extension'] == 'htm' ) )
            {
               $contents = file_get_contents( $newPath ) ;
               if ( $contents === false )
               {
                  return false ;
               }
               $contents = str_replace( "\n", "\r\n", $contents ) ;
               file_put_contents( $newPath, $contents ) ;
            }
         }
      }
   }
   return true ;
}

function copyDir( $src, $dst )
{
   if( is_dir( $src ) == false )
   {
      return ;
   }
   $handle = @opendir( $src ) ;
   @mkdir( $dst, 0777, true ) ;
   chmod( $dst, 0777 ) ;
   while( ( $file = @readdir( $handle ) ) != false )
   {
      if( $file != '.' && $file != '..' )
      {
         if( is_dir( "$src/$file" ) )
         {
            copyDir( "$src/$file", "$dst/$file" ) ;
         }
         else
         {
            copy( "$src/$file", "$dst/$file" ) ;
            chmod( "$dst/$file", 0777 ) ;
         }
      }
   }
   closedir( $handle ) ;
}

function clearDir( $path )
{
   if( removeDir( $path ) )
   {
      $rc = mkdir( $path, 0777, true ) ;
      chmod( $path, 0777 ) ;
      return $rc ;
   }
   return false ;
}

function printLog( $errMsg, $type = "Error" )
{
   echo 'PHP '.$type.': '.$errMsg."\n" ;
}

function execCmd( $cmd, $isPrint = false )
{
   $output = array() ;
   $return_val = 0 ;
   exec( $cmd, $output, $return_val ) ;
   if( $return_val != 0 || $isPrint == true )
   {
      $errMsg = "" ;
      foreach( $output as $index => $line )
      {
         $errMsg = $errMsg."   ".$line."\n" ;
      }
      printLog( $errMsg, $return_val == 0 ? 'Event' : 'Error' ) ;
   }
   return $return_val ;
}

function iterDoxygenConfig( $path )
{
   $list = array() ;
   $res = dir( $path ) ;
   while( $file = $res->read() )
   {
      if( $file != '.' && $file != '..' )
      {
         if( is_dir( getOSInfo() == 'linux' ? "$path/$file" : "$path\\$file" ) == false )
         {
            $parts = pathinfo( $file ) ;
            if( $parts['extension'] == 'conf' )
            {
               array_push( $list, "config/doxygen/$file" ) ;
            }
         }
      }
   }
   return $list ;
}

function buildMdConverter( $toolPath, $originPath )
{
   $mdConverterFile = getOSInfo() == 'linux' ? "linux_mdConverter" : "mdConverter.exe" ;
   $mdConverterPath = $toolPath. DIRECTORY_SEPARATOR .$mdConverterFile ;

   if( file_exists( $mdConverterPath ) )
   {
      @unlink( $mdConverterPath ) ;
   }

   $mdConverterSrc = $toolPath. DIRECTORY_SEPARATOR ."mdConverter" ;

   if ( FALSE == chdir( $mdConverterSrc ) )
   {
      printLog( "Failed to chdir build path" ) ;
      return FALSE ;
   }

   $mdConverterBuildFile = $mdConverterSrc. DIRECTORY_SEPARATOR .( getOSInfo() == 'linux' ? "build.sh" : "build.bat" ) ;

   chmod( $mdConverterBuildFile, 0777 ) ;

   if( execCmd( $mdConverterBuildFile ) != 0 )
   {
      printLog( "Failed to build mdConverter" ) ;
      return FALSE ;
   }

   $mdConverterBuildPath = $mdConverterSrc. DIRECTORY_SEPARATOR ."build" ;
   $mdConverterNewFile = $mdConverterBuildPath. DIRECTORY_SEPARATOR .( getOSInfo() == 'linux' ? "linux_mdConverter" : "mdConverter.exe" ) ;

   copy( $mdConverterNewFile, $mdConverterPath ) ;
   chmod( $mdConverterPath, 0777 ) ;

   if ( FALSE == chdir( $originPath ) )
   {
      printLog( "Failed to restore work path" ) ;
      return FALSE ;
   }

   return TRUE ;
}
