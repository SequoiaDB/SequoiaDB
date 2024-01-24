/****************************************************
@description:      random write lob
@testlink cases:   seqDB-13456
@modify list:
        2018-3-13 huangxiaoni init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest13456 extends PHPUnit_Framework_TestCase
{
   private static $LobUtils;
   private static $skipTestCase = false;
   
   private static $db;
   private static $cl;
   private static $csName = "php13456";
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
         echo "Failed to exec selesctCL, error code: ".$err['errno'];
         self::$skipTestCase = true;
         return;
      }
      
      // new LobUtils
      self::$LobUtils = new LobUtils( self::$db, self::$cl );
      self::$oid = self::$LobUtils -> getOid();
      //var_dump("oid = ".self::$oid);
      self::$writeStr = self::$LobUtils -> getRandomStr( self::$writeLen );
   }
   
   public function setUp()
   {
      if ( self::$skipTestCase == true )
      {
         $this->markTestSkipped( 'init failed' );
      }
   }
   
   public function test_writeLobAndGetModifyTime()
   {
      echo "\n---Begin to write lob[ lock, seek ].\n"; 
      
      echo "   Begin to open lob.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_CREATEONLY );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to seek lob.\n"; 
      $offset = 0;
      $whence = SDB_LOB_SET;
      $err = $lobObj -> seek( $offset, $whence );
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "\n   Begin to getModificationTime after seek.\n"; 
      $createTime = $lobObj -> getCreateTime();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );       
      $modifyTime = $lobObj -> getModificationTime();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );       
      $this -> assertEquals( $createTime, $modifyTime ); 
      
      echo "\n   Begin to write lob.\n"; 
      $err = $lobObj -> write( self::$writeStr );
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to close lob.\n"; 
      $err = $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "\n   Begin to check the lob.\n";
      self::$LobUtils -> checkLobContent( self::$oid, self::$writeLen, self::$writeStr );
      
      echo "\n   Begin to getModificationTime after write and close.\n"; 
      echo "   Begin to open lob.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_READ );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
        
      echo "   Begin to getModificationTime again.\n"; 
      $modifyTime2 = $lobObj -> getModificationTime();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );    
      $this -> assertGreaterThan( $modifyTime, $modifyTime2 );
      
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