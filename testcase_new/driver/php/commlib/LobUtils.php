/****************************************************
@description:      lob operate, warp class
@testlink cases:   seqDB-7681-7689
@modify list:
        2018-3-14 huangxiaoni init
****************************************************/
<?php
class LobUtils
{
   private $db;
   private $cl;
      
   public function __construct( $db, $cl )
   {
      $this -> db = $db;
      $this -> cl = $cl;
   }
      
   public function getOid()
   {
      $uniqid = uniqid();
      $md5 = md5( $uniqid );     
      $oid = substr($md5, 8);
      return $oid;
   }
   
   public function getBytes( $len )
   {
      $str = $this -> getRandomStr( $len );
      $bytes = array();
      for( $i = 0; $i < $len; $i++ )
      {
         $bytes[] = ord( $str[$i] );
      }
      return $bytes;
   }
   
   public function getRandomStr( $len )
   {
      $str = "";
      for( $i = 0; $i < $len; $i++ )
      {
         $str .= chr( mt_rand(33, 126) );
      }
      return $str;
   }
   
   public function writeLob( $oid, $str )
   {
      $lobObj = $this -> cl -> openLob( $oid, SDB_LOB_CREATEONLY );
      $errno = $this -> db -> getError()['errno'];
      if( 0 !== $errno )
      {
         throw new Exception( "errno = ".$errno.", failed to open the lob[".$oid."]." );
      }
      
      $err = $lobObj -> write( $str );
      $errno = $this -> db -> getError()['errno'];
      if( 0 !== $errno )
      {
         throw new Exception( "errno = ".$errno.", failed to write the lob[".$oid."]." );
      }
      
      $err = $lobObj -> close();
      $errno = $this -> db -> getError()['errno'];
      if( 0 !== $errno )
      {
         throw new Exception( "errno = ".$errno.", failed to close the lob[".$oid."]." );
      }
   }
      
   public function checkLobExist( $oid )
   {
      $isExist = false;
      $cursor = $this -> cl -> listLob();
      $errno = $this -> db -> getError()['errno'];
      if( 0 !== $errno )
      {
         throw new Exception( "errno = ".$errno.", failed to exec listLob." );
      }
          
      while( $record = $cursor -> next() )
      {
         if( $record['Oid'] == $oid )
         {
            $isExist = true;
            break;
         }
      }
      
      if( !$isExist )
      {
         throw new Exception( "failed to check lob[".$oid."], expect lob is exist, but not exist." );
      }
   }
      
   public function checkLobContent( $oid, $readLen, $expStr )
   {
      $lobObj = $this -> cl -> openLob( $oid, SDB_LOB_READ );
      $errno = $this -> db -> getError()['errno'];
      if( 0 !== $errno )
      {
         throw new Exception( "errno = ".$errno.", failed to open the lob[".$oid."]." );
      }
            
      $readStr = $lobObj -> read( $readLen );
      $errno = $this -> db -> getError()['errno'];
      if( 0 !== $errno )
      {
         throw new Exception( "errno = ".$errno.", failed to read the lob[".$oid."]." );
      }
      //echo "----readStr-----\n";
      //var_dump($expStr);
      //var_dump($readStr);
      
      $err = $lobObj -> close();
      $errno = $this -> db -> getError()['errno'];
      if( 0 !== $errno )
      {
         throw new Exception( "errno = ".$errno.", failed to close the lob[".$oid."]." );
      }
      
      $actMd5 = md5( $readStr );
      $expMd5 = md5( $expStr );
      if( strcmp($actMd5, $expMd5 ) )
      {
         throw new Exception( "failed to check lob[".$oid."] \nactMd5 = ".$actMd5.", expMd5 = ".$expMd5 );
      }
   }
   
}
?>