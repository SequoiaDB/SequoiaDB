/******************************************************************************
*@Description : seqDB-22396:验证getRanTimeDetail接口
*@Author:      2020/08/11  liuli
******************************************************************************/
<?php 

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest22396 extends PHPUnit_Framework_TestCase
{
   private static $LobUtils;
   private static $db;
   private static $cl;
   private static $csName = "cs_22396";
   private static $clName = "cl_22396";
   private static $oid;
   private static $writeLen = 100;
   private static $writeStr;
   
   public function setUp()
   {
      self::$db = new SequoiaDB();
      self::$db -> connect( globalParameter::getHostName(), globalParameter::getCoordPort() );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $cs = self::$db -> selectCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      self::$cl = $cs -> selectCL( self::$clName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      // new LobUtils
      self::$LobUtils = new LobUtils( self::$db, self::$cl );
      self::$oid = self::$LobUtils -> getOid();
      self::$writeStr = self::$LobUtils -> getRandomStr( self::$writeLen );
      self::$LobUtils -> writeLob( self::$oid, self::$writeStr );
   }
   
   function test_shareRead()
   {
      echo "\n---Begin to share read lob[ lock and seek ].\n";
      
      echo "   Begin to open lob and share read.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_SHAREREAD );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to lock and seek lob.\n"; 
      $offset = 5;
      $len = 20;
      $err = $lobObj -> lockAndSeek( $offset, $len );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to get the run time detail information of lob.\n";
      $detail = $lobObj -> getRunTimeDetail();
      $accessInfo = $detail[ 'AccessInfo' ];
      unset( $accessInfo['LockSections'][0]['Contexts'] );
      $expect = array( 'RefCount' => 1,
      'ReadCount' => 0,
      'WriteCount' => 0,
      'ShareReadCount' => 1,
      'LockSections' => array( 0 => array( 'Begin' => 5,
      'End' => 25,
      'LockType' => 'S')));
      $this -> assertEquals( $expect, $accessInfo );
      
      echo "   Begin to close lob.\n";
      $err = $lobObj -> close();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   function test_shareReadWrite()
   {
      echo "\n---Begin to share read and write lob[ lock and seek ].\n";
      
      echo "   Begin to open lob and share read and write.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_SHAREREAD | SDB_LOB_WRITE );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to lock and seek lob.\n"; 
      $offset = 10;
      $len = 20;
      $err = $lobObj -> lockAndSeek( $offset, $len );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );      
      
      echo "   Begin to get the run time detail information of lob.\n";
      $detail = $lobObj -> getRunTimeDetail();
      $accessInfo = $detail[ 'AccessInfo' ];
      unset( $accessInfo['LockSections'][0]['Contexts'] );
      $expect = array( 'RefCount' => 1,
      'ReadCount' => 0,
      'WriteCount' => 1,
      'ShareReadCount' => 0,
      'LockSections' => array( 0 => array( 'Begin' => 10,
      'End' => 30,
      'LockType' => 'X')));
      $this -> assertEquals( $expect, $accessInfo );
      
      echo "   Begin to close lob.\n";
      $err = $lobObj -> close();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   public function tearDown()
   {
      echo "\n---Clean up the path.\n";
      $err = self::$db -> dropCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$db -> close();
   }
}
?>