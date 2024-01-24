/****************************************************
@description:      lob truncate
@testlink cases:   seqDB-13461
@modify list:
        2018-3-13 huangxiaoni init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest13461 extends PHPUnit_Framework_TestCase
{
   private static $LobUtils;
   private static $skipTestCase = false;
   
   private static $db;
   private static $cl;
   private static $csName = "php13461";
   private static $clName = "cl";
   
   private static $oid;
   private static $lobLen = 10;
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
      self::$writeStr = self::$LobUtils -> getRandomStr( self::$lobLen );
      //var_dump("str = ".self::$writeStr);
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
      
      echo "   Begin to open lob.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_CREATEONLY );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to write lob.\n"; 
      $err = $lobObj -> write( self::$writeStr );
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to close lob.\n"; 
      $err = $lobObj -> close();
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to check the lob.\n";
      self::$LobUtils -> checkLobExist( self::$oid );
      self::$LobUtils -> checkLobContent( self::$oid, self::$lobLen, self::$writeStr );
   }
   
   public function test_truncateLob01()
   {
      echo "\n---Begin to truncate lob[ length = lobLen ].\n"; 
      $len = self::$lobLen;
      $expStr = self::$writeStr;
      
      $err = self::$cl -> truncateLob( self::$oid, $len);
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to check the lob.\n";
      self::$LobUtils -> checkLobContent( self::$oid, $len, $expStr );
   }
   
   public function test_truncateLob02()
   {
      echo "\n---Begin to truncate lob[ length = (lobLen+1) ].\n"; 
      $len = self::$lobLen + 1;
      $expStr = self::$writeStr;
            
      $err = self::$cl -> truncateLob( self::$oid, $len);
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to check the lob.\n";
      self::$LobUtils -> checkLobContent( self::$oid, $len, $expStr );
   }
   
   public function test_truncateLob03()
   {
      echo "\n---Begin to truncate lob[ length = (lobLen-1) ].\n"; 
      $len = self::$lobLen - 1;
      $expStr = subStr( self::$writeStr, 0, $len );
      //var_dump($expStr);
      
      $err = self::$cl -> truncateLob( self::$oid, $len);
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to check the lob.\n";
      self::$LobUtils -> checkLobContent( self::$oid, $len, $expStr );
   }
   
   public function test_truncateLob04()
   {
      echo "\n---Begin to truncate lob[ length = 0 ].\n"; 
      $len = 0;
      $expStr = subStr( self::$writeStr, 0, $len );
      //var_dump($expStr);
      
      $err = self::$cl -> truncateLob( self::$oid, $len);
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to check the lob.\n";
      self::$LobUtils -> checkLobExist( self::$oid );
      self::$LobUtils -> checkLobContent( self::$oid, $len, $expStr );
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