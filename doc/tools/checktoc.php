<?php

include_once "html2mysql/parseConf.php" ;
include_once "html2mysql/function.php" ;

$root = dirname( __FILE__ ).( getOSInfo() == 'linux' ? "/.." : "\\.." ) ;
$rootPath = '' ;
$path = getOSInfo() == 'linux' ? "$root/config/toc.json" : "$root\\config\\toc.json" ;

$config = getConfig( $path ) ;
if( $config == FALSE )
{
   exit( 1 ) ;
}

if( getOSInfo() == 'windows' )
{
   $rootPath = "$root\\src\\manual" ;
}
else
{
   $rootPath = "$root/src/manual" ;
}

check( $rootPath, $config ) ;

function check( $path, $config )
{
   $files = scandir( $path ) ;

   foreach( $files as $file )
   {
      if( $file=='.' || $file=='..' || $file=='Readme.md' )
      {
         continue;
      }

      $newPath = $path ;
      $filename = $file ;
      $next = null ;

      if( !array_key_exists( 'contents', $config ) )
      {
         echo 'Is not dir, id='.$config['id'] ;
         return false ;
      }

      if( getOSInfo() == 'windows' )
      {
         $newPath = $newPath."\\".$file ;
      }
      else
      {
         $newPath = $newPath."/".$file ;
      }

      $isDir = is_dir( $newPath ) ;
      if( !$isDir )
      {
         $info = pathinfo( $file ) ;
         $filename = $info['filename'] ;
      }

      $isFind = false ;

      if( strpos( $file, "_en" ) !== false )
      {
         $isFind = true ;
      }

      foreach( $config['contents'] as $contentInfo )
      {
         if ( $isDir )
         {
            if( array_key_exists( 'dir', $contentInfo ) && $contentInfo['dir'] == $filename )
            {
               $isFind = true ;
               $next = $contentInfo ;
               break ;
            }
         }
         else
         {
            if( array_key_exists( 'file', $contentInfo ) && $contentInfo['file'] == $filename )
            {
               $isFind = true ;
               break ;
            }
         }
      }

      if( !$isFind )
      {
         echo "Not exist: path=$newPath, id=".$config['id']."\r\n" ;
      }


      if( $isDir && $isFind )
      {
         if( !check( $newPath, $next ) )
         {
            //return false ;
         }
      }
   }

   return true ;
}