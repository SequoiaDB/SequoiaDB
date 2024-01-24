/****************************************************
@description:      random write lob
@testlink cases:   seqDB-13452
@modify list:
        2018-3-13 huangxiaoni init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest13452 extends PHPUnit_Framework_TestCase
{
   private static $LobUtils;
   private static $skipTestCase = false;
   
   private static $db;
   private static $cl;
   private static $csName = "php13452";
   private static $clName = "cl";
   
   private static $oid;
   private static $lobLen = 10;
   private static $offset;
   private static $writeStr;
   private static $writeStr2;
   
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
         echo "Failed to exec selesctCL, error code: ".$err['errno'];
         self::$skipTestCase = true;
         return;
      }
      
      // new LobUtils
      self::$LobUtils = new LobUtils( self::$db, self::$cl );
      self::$oid = self::$LobUtils -> getOid();
      //var_dump("oid = ".self::$oid);
      self::$writeStr  = self::$LobUtils -> getRandomStr( self::$lobLen );
   }
   
   public function setUp()
   {
      if ( self::$skipTestCase == true )
      {
         $this->markTestSkipped( 'init failed' );
      }
   }
   
   public function test_writeLob()
   {
      echo "\n---Begin to write lob.\n"; 
      $mode = SDB_LOB_CREATEONLY;
      self::$offset = 2;
      $whence = SDB_LOB_SET;
      self::$writeStr2 = self::$LobUtils -> getRandomStr( self::$lobLen - self::$offset );
      
      $this -> writeBySeek( $mode, self::$offset, $whence, self::$writeStr2 );
      $this -> readBySeek( self::$offset, $whence, self::$writeStr2 );
      $this -> readLobInfo();
   }
   
   public function test_writeLobAgain()
   {
      echo "\n---Begin to write lob again.\n"; 
      $mode = SDB_LOB_WRITE;
      $offset3 = 4;
      $whence = SDB_LOB_SET;
      $writeStr3 = self::$LobUtils -> getRandomStr( self::$lobLen - $offset3 );
      
      $this -> writeBySeek( $mode, $offset3, $whence, $writeStr3 );
      $this -> readBySeek( $offset3, $whence, $writeStr3 );
      $this -> readLobInfo();
      
      $tmpStr = subStr( self::$writeStr2, 0, self::$offset ).$writeStr3;
      $this -> readBySeek( $offset3 - self::$offset, $whence, $tmpStr );
   }
   
   public function writeBySeek( $mode, $offset, $whence, $writeStr2 )
   {
      echo "   Begin to open lob[ ".$mode.", seek ].\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, $mode );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to seek lob.\n"; 
      $err = $lobObj -> seek( $offset, $whence );
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to write lob.\n"; 
      $err = $lobObj -> write( $writeStr2 );
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to close lob.\n"; 
      $err = $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] ); 
   }
   
   public function readBySeek( $offset, $whence, $writeStr2 )
   {
      echo "\n   Begin to read lob.\n";
            
      echo "   Begin to open lob[ SDB_LOB_READ, seek ].\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_READ );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to seek lob.\n"; 
      $err = $lobObj -> seek( $offset, $whence );
      $this -> assertEquals( 0, $err['errno'] );
      
      $newStr = $lobObj -> read( self::$lobLen - $offset );
      $this -> assertEquals( 0, $err['errno'] );
      //var_dump($newStr);
      
      echo "   Begin to close lob.\n"; 
      $err = $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to check results.\n"; 
      $this -> assertEquals( md5( $newStr ), md5( $writeStr2 ) );
   }
   
   public function readLobInfo()
   {
      echo "\n   Begin to read lob info.\n";       
      
      echo "   Begin to open lob.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_READ );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      echo "   Begin to get lob info.\n";
      $size = $lobObj -> getSize();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      $this -> assertEquals( $size, self::$lobLen ); 
      
      $createTime = $lobObj -> getCreateTime();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );       
      $modifyTime = $lobObj -> getModificationTime();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );       
      $this -> assertGreaterThan( $createTime, $modifyTime ); 
      
      echo "   Begin to close lob.\n"; 
      $err = $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] );
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