<?php

function getOSInfo()
{
   return (DIRECTORY_SEPARATOR == '\\') ? 'windows' : 'linux' ;
}

function sendStreamFile( $url, $file, $dest )
{
   $dest = str_replace( "\\", "/", $dest ) ;
   $dest = str_replace( "//", "/", $dest ) ;
   if( file_exists( $file ) )
   {
      $opts = array(
         'http' => array(
            'method' => 'POST',
            'header' => 'Content-Type:application/x-www-form-urlencoded',
            'content' => file_get_contents( $file )
         ),
         'ssl' => array(
            'verify_peer' => false,
            'verify_peer_name' => false
         )
      ) ;
      $url = $url.'?cmd=file&dest='.urlEncode( $dest ) ;
      $context = stream_context_create( $opts ) ;
      $respone = file_get_contents( $url, false, $context ) ;
      $ret = json_decode( $respone, true ) ;
      return $ret ;
   }
   else
   {
      return false ;
   }
}

function sendMkdir( $url, $dest )
{
   $dest = str_replace( "\\", "/", $dest ) ;
   $dest = str_replace( "//", "/", $dest ) ;
   $opts = array(
      'http' => array(
         'method' => 'POST',
         'header' => 'Content-Type:application/x-www-form-urlencoded'
      ),
      'ssl' => array(
         'verify_peer' => false,
         'verify_peer_name' => false
      )
   ) ;
   $url = $url.'?cmd=mkdir&dest='.urlEncode( $dest ) ;
   $context = stream_context_create( $opts ) ;
   $respone = file_get_contents( $url, false, $context ) ;
   $ret = json_decode( $respone, true ) ;
   return $ret ;
}

function iterDir( $root, $path, $edition, $type )
{
   if( $type == 'document' )
   {
      sendMkdir( 'https://www.sequoiadb.com/cn/receiveImage.php',
                 "./index/Public/Home/$type/$edition" ) ;
   }
   $ret = sendMkdir( 'https://www.sequoiadb.com/cn/receiveImage.php',
                     $type == 'document' ? "./index/Public/Home/$type/$edition/api/$path" : "./index/Public/Home/$type/$edition/$path" ) ;
   if( $ret['errno'] != true )
   {
      echo "Failed to create dir, path: $root/$path\n" ;
      echo $type == 'document' ? "Dest path: ./index/Public/Home/$type/$edition/api/$path\n" : "Dest path: ./index/Public/Home/$type/$edition/$path\n" ;
      return false ;
   }
   $res = dir( "$root/$path" ) ;
   while( $file = $res->read() )
   {
      if( $file != '.' && $file != '..' )
      {
         if( is_dir( getOSInfo() == 'linux' ? "$root/$path/$file" : "$root\\$path\\$file" ) )
         {
            $ret = iterDir( $root, getOSInfo() == 'linux' ? "$path/$file" : "$path\\$file", $edition, $type ) ;
            if( $ret == false )
            {
               return false ;
            }
         }
         else
         {
            $ret = sendStreamFile( 'https://www.sequoiadb.com/cn/receiveImage.php', getOSInfo() == 'linux' ? "$root/$path/$file" : "$root\\$path\\$file",
                                   $type == 'document' ? "./index/Public/Home/$type/$edition/api/$path/$file" : "./index/Public/Home/$type/$edition/$path/$file" ) ;
            if( $ret['errno'] != true )
            {
               echo "Failed to send image, path: $root/$path/$file\n" ;
               echo $type == 'document' ? "Dest path: ./index/Public/Home/$type/$edition/api/$path/$file\n" : "Dest path: ./index/Public/Home/$type/$edition/$path/$file\n" ;
               return false ;
            }
         }
      }
   }
   return true ;
}