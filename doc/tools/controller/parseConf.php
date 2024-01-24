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

function checkConfig( $conf, $arr )
{
   if( !is_array( $arr ) )
   {
      $arr = array() ;
   }

   $contents = $conf['contents'] ;
   foreach( $contents as $value )
   {
      $hasId = array_key_exists( 'id', $value ) ;
      $hasFile = array_key_exists( 'file', $value ) ;
      $haDir = array_key_exists( 'dir', $value ) ;
      $hasContents = array_key_exists( 'contents', $value ) ;

      if ( !$hasId )
      {
         $tips = '' ;
         if( $hasFile )
         {
            $tips = 'file='.$value['file'] ;
         }
         else if( $haDir )
         {
            $tips = 'dir='.$value['dir'] ;
         }
         $error = sprintf( 'Error: id does not exist, %s', $tips ) ;
         throw new Exception( $error ) ;
      }

      $id = $value['id'] ;

      if( in_array( $id, $arr ) )
      {
         $error = sprintf( "Error: id %d repeats", $id ) ;
         throw new Exception( $error ) ;
      }

      if ( $hasFile && $haDir )
      {
         $error = sprintf( "Error: Key and dir cannot exist at the same time, id=%d", $id ) ;
         throw new Exception( $error ) ;
      }

      if ( $haDir && !$hasContents )
      {
         $error = sprintf( "Error: Missing contents field, id=%d", $id ) ;
         throw new Exception( $error ) ;
      }

      if ( $haDir && !is_array( $value['contents'] ) )
      {
         $error = sprintf( "Error: The contents field must be an array, id=%d", $id ) ;
         throw new Exception( $error ) ;
      }

      array_push( $arr, $id ) ;

      if ( $haDir )
      {
         checkConfig( $value, $arr ) ;
      }
   }
}

function getVersion( $path )
{
   return getConfig( $path ) ;
}

function getCnPath( $id, $config, $path )
{
   if( array_key_exists( 'contents', $config ) )
   {
      if( array_key_exists( 'id', $config ) && $config['id'] == $id )
      {
         return $path ;
      }
      else
      {
         foreach( $config['contents'] as $index => $value )
         {
            $newPath = getCnPath( $id, $value, "$path/".$value['cn'] ) ;
            if( $newPath !== false )
            {
               return $newPath ;
            }
         }
      }
   }
   return false ;
}

function getDirPath( $id, $config, $path )
{
   if( array_key_exists( 'contents', $config ) )
   {
      if( array_key_exists( 'id', $config ) && $config['id'] == $id )
      {
         return $path ;
      }
      else
      {
         foreach( $config['contents'] as $index => $value )
         {
            $filename = array_key_exists( 'dir', $value ) ? $value['dir'] : $value['file'] ;
            $newPath = getDirPath( $id, $value, "$path/$filename" ) ;
            if( $newPath !== false )
            {
               return $newPath ;
            }
         }
      }
   }
   return false ;
}