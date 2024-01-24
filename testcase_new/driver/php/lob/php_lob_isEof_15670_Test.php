/****************************************************
@description:      test lob interface isEof
@testlink cases:   seqDB-15670
@modify list:
        2018-9-3   linsuqiang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest15670 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = "cs15670";
   private static $clName = "cl15670";
   private static $cl;
   private static $LobUtils;
   private static $oid;
   private static $writeStr;
   private static $writeLen = 1025;

   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      self::checkErrno( 0, self::$db -> getError()['errno'] );

      $cs = self::$db -> selectCS( self::$csName );
      self::checkErrno( 0, self::$db -> getError()['errno'] );

      self::$cl = $cs -> selectCL( self::$clName );
      self::checkErrno( 0, self::$db -> getError()['errno'] );

      self::$LobUtils = new LobUtils( self::$db, self::$cl );
      self::$oid = self::$LobUtils -> getOid();
      self::$writeStr = self::$LobUtils -> getRandomStr( self::$writeLen );
      self::$LobUtils -> writeLob( self::$oid, self::$writeStr );
   }

   public function test()
   {
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_READ );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      
      $rData = "";
      $buffer = "";
      $bufferLen = 128;
      while( !$lobObj -> isEof() )
      {
         $buffer = $lobObj -> read( $bufferLen );
         self::checkErrno( 0, self::$db -> getError()['errno'] );
         $rData .= $buffer;
      }
      $lobObj -> close();
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      
      $actMd5 = md5( $rData );
      $expMd5 = md5( self::$writeStr );
      $this -> assertEquals( $actMd5, $expMd5 );
   }
   
   public static function tearDownAfterClass()
   {
      $err = self::$db -> dropCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to drop cs, errno=".$err['errno']);
      }
      
      self::$db->close();
   }

   private static function checkErrno( $expErrno, $actErrno, $msg = "" )
   {
      if( $expErrno != $actErrno ) 
      {
         throw new Exception( "expect [".$expErrno."] but found [".$actErrno."]. ".$msg );
      }
   }
   
}
?>
