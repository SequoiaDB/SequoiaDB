/****************************************************
@description:      random write lob
@testlink cases:   seqDB-14796
@modify list:
        2018-3-13 huangxiaoni init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest14796 extends PHPUnit_Framework_TestCase
{
   private static $LobUtils;
   private static $skipTestCase = false;
   
   private static $db;
   private static $cl;
   private static $csName = "php14796";
   private static $clName = "cl";
   
   private static $oid;
   private static $writeLen = 100;
   private static $writeStr;
   
   public static function setUpBeforeClass()
   {        
      // connect sdb
      self::$db = new SequoiaDB();
      $err = self::$db -> connect( globalParameter::getHostName(), globalParameter::getCoordPort() );
      if ( $err['errno'] != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'];
         self::$skipTestCase = true;
         return;
      }   
      
      // setSssionAttr['m']
      $err = self::$db -> setSessionAttr(array('PreferedInstance' => 'm' ));
      if ( $err['errno'] != 0 )
      {
         echo "Failed to call setSessionAttr, error code: ".$err['errno'];
         self::$skipTestCase = true;
         return;
      }
      
      // create cs
      $cs = self::$db -> selectCS( self::$csName );
      $err = self::$db -> getError();
      if( $err['errno'] != 0 ) 
      {
         echo "Failed to exec selectCS, error code: ".$err['errno'];
         self::$skipTestCase = true;
         return;
      }
      
      // create cl
      self::$cl = $cs -> selectCL( self::$clName );
      $err = self::$db -> getError();
      if( $err['errno'] != 0 ) 
      {
         echo "Failed to exec selectCL, error code: ".$err['errno'];
         self::$skipTestCase = true;
         return;
      }
      
      // new LobUtils
      self::$LobUtils = new LobUtils( self::$db, self::$cl );
      self::$oid = self::$LobUtils -> getOid();
      //var_dump("oid = ".self::$oid);
      self::$writeStr = self::$LobUtils -> getRandomStr( self::$writeLen );
      self::$LobUtils -> writeLob( self::$oid, self::$writeStr );
   }
   
   public function setUp()
   {
      if ( self::$skipTestCase == true )
      {
         $this->markTestSkipped( 'init failed' );
      }
   }
   
   public function test_seekAndReadLobForSet()
   {
      echo "\n---Begin to read lob.\n";
      $offset = 1;
       
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_READ );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $err = $lobObj -> seek( $offset, SDB_LOB_SET );
      $this -> assertEquals( 0, $err['errno'] );
      
      $rData = $lobObj -> read( self::$writeLen );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] );
      
      $actMd5 = md5( $rData );
      $expMd5 = md5( subStr(self::$writeStr, $offset, self::$writeLen ) );
      $this -> assertEquals( $actMd5, $expMd5 );
      //var_dump("rData = ".$rData);
   }
   
   public function test_seekAndReadLobForCur()
   {
      echo "\n---Begin to read lob[1, SDB_LOB_CUR].\n"; 
      $offset = 1;
      
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_READ );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $err = $lobObj -> seek( $offset, SDB_LOB_CUR );
      $this -> assertEquals( 0, $err['errno'] );
      
      $rData = $lobObj -> read( self::$writeLen );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $err = $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] );
      
      $actMd5 = md5( $rData );
      $expMd5 = md5( subStr(self::$writeStr, $offset, self::$writeLen ) );
      $this -> assertEquals( $actMd5, $expMd5 );
   }
   
   public function test_seekAndReadLobForEnd()
   {
      echo "\n---Begin to read lob[1, SDB_LOB_END].\n"; 
      $offset = 1;
      
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_READ );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $err = $lobObj -> seek( $offset, SDB_LOB_END );
      $this -> assertEquals( 0, $err['errno'] );
      
      $rData = $lobObj -> read( self::$writeLen );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $err = $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] );      
      
      $actMd5 = md5( $rData );
      $expMd5 = md5( subStr(self::$writeStr, self::$writeLen - $offset, self::$writeLen ) );
      //var_dump("wData = \n".self::$writeStr."\n");
      //var_dump("rData = \n".$rData."\n");
      //var_dump("eData = \n".subStr(self::$writeStr, self::$writeLen - $offset, self::$writeLen )."\n");
      $this -> assertEquals( $actMd5, $expMd5 );
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to dropCS in the end.\n"; 
      $err = self::$db -> dropCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to drop cs, errno=".$err['errno']);
      }
      
      $err = self::$db->close();
   }
   
}
?>