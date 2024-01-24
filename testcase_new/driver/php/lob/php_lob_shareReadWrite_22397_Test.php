/******************************************************************************
*@Description : seqDB-22397:以 SHARE_READ、SHARE_READ|WRITE 模式读写 lob
*@Author:      2020/08/13  liuli
******************************************************************************/
<?php 

include_once dirname(__FILE__).'/../commlib/LobUtils.php';
include_once dirname(__FILE__).'/../global.php';
class LobTest22397 extends PHPUnit_Framework_TestCase
{
   private static $LobUtils;
   private static $db;
   private static $cl;
   private static $csName = "cs_22397";
   private static $clName = "cl_22397";
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
   
   function test_shareReadWrite()
   {
      echo "\n---Begin to share read and write lob.\n";
      
      echo "   Begin to open lob and share read and write.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid,  SDB_LOB_SHAREREAD | SDB_LOB_WRITE );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to read lob.\n"; 
      $read = $lobObj -> read( self::$writeLen );
      $this -> assertEquals( $read, self::$writeStr );
      
      echo "   Begin to write lob.\n"; 
      $err = $lobObj -> write( 'hello' );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to close lob.\n";
      $err = $lobObj -> close();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
       echo "   Begin to open lob and share read.\n"; 
      $lobObj = self::$cl -> openLob( self::$oid, SDB_LOB_SHAREREAD );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      echo "   Begin to read lob.\n"; 
      $read = $lobObj -> read( self::$writeLen + 5 );
      $this -> assertEquals( $read, self::$writeStr . 'hello' );
      
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