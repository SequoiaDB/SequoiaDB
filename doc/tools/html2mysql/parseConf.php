<?php

function getConfig( $path )
{
   if( file_exists( $path ) === false )
   {
      echo "No such file, $path\n" ;
      return false ;
   }
   $json = file_get_contents( $path ) ;
   if( $json === false )
   {
      echo "Failed to get file contents, $path\n" ;
      return false ;
   }
   $arr = json_decode( $json, true ) ;
   if( $arr == NULL )
   {
      echo "Failed to decode json, $path\n" ;
      return false ;
   }
   return $arr ;
}

function getVersion( $path )
{
   return getConfig( $path ) ;
}