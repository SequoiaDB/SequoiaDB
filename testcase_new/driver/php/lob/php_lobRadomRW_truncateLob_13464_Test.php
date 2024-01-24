/****************************************************
@description:      lob truncate
@testlink cases:   seqDB-13464
@modify list:
        2018-3-13 huangxiaoni init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest13464 extends PHPUnit_Framework_TestCase
{
   private static $LobUtils;
   private static $skipTestCase = false;
   
   private static $db;
   private static $cl;
   private static $csName = "php13464";
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
      self::$LobUtils -> writeLob( self::$oid, self::$writeStr );
   }
   
   public function setUp()
   {
      if ( self::$skipTestCase == true )
      {
         $this->markTestSkipped( 'init failed' );
      }
   }
   
   public function test_truncateLob01()
   {
      echo "\n---Begin to truncate lob[ non-exist oid ].\n"; 
      $unexistOid = self::$LobUtils -> getOid();
      $err = self::$cl -> truncateLob( $unexistOid, self::$lobLen);
      $this -> assertEquals( -4, $err['errno'] );
   }
   
   public function test_truncateLob02()
   {
      echo "\n---Begin to truncate lob[ avalid oid ].\n"; 
      $avalidOid = "abcd000";            
      $err = self::$cl -> truncateLob( $avalidOid, self::$lobLen);
      $this -> assertEquals( -4, $err['errno'] );
   }
   
   public function test_truncateLob03()
   {
      echo "\n---Begin to truncate lob[ avalid length[-1] ].\n";       
      $err = self::$cl -> truncateLob( self::$oid, -1);
      $this -> assertEquals( -6, $err['errno'] );
   }
   
   public function test_truncateLob04()
   {
      echo "\n---Begin to truncate lob[ length avalid(e.g:'test') ].\n"; 
      $err = self::$cl -> truncateLob( self::$oid, "test111");
      $this -> assertEquals( -6, $err['errno'] );   
   }
   
   /* SequoiaINT64 is wrong when it goes beyond the int64 boundary, so the truncateLob point is not measured      
   public function test_truncateLob05()
   {
      echo "\n---Begin to truncate lob[ avalid length[beyond the long boundary(9223372036854775808)] ].\n";  
      $readLen = new SequoiaINT64( '9223372036854775808' );
      $this -> assertEquals( -6, self::$db -> getError()['errno'] ); 
      
      //$err = self::$cl -> truncateLob( self::$oid, $readLen );
      //$this -> assertEquals( -6, $err['errno'] );
   }*/
   
   public function test_truncateLob06()
   {
      echo "\n---Begin to truncate lob[ length = int64 boundary(9223372036854775807) ].\n"; 
      $len = new SequoiaINT64( '9223372036854775807' );
      $err = self::$cl -> truncateLob( self::$oid, $len);
      $this -> assertEquals( 0, $err['errno'] );
      
      echo "   Begin to check the lob.\n";
      self::$LobUtils -> checkLobContent( self::$oid, self::$lobLen, self::$writeStr );
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