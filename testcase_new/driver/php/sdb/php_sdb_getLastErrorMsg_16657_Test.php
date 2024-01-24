/****************************************************
@description:      check error msg
@testlink cases:   seqDB-16657
@modify list:
        2018-11-19 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class GetLastErrMsg16657 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs16657";
   private static $clName = "cl16657";
   private static $cl;
   private static $db;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );                     
                           
      $cs = self::$db -> selectCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$cl = $cs -> selectCL( self::$clName );	
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$cl -> createIndex( array( 'a' => 1), "myIndex", true );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   function test()
   {
      echo "\n---Begin to check getError.\n";
      self::$cl -> insert( array( 'a' => 1) );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$cl -> insert( array( 'a' => 1) );
      $errMsg1 = self::$db -> getLastErrorMsg();
      $this -> assertEquals( $errMsg1["errno"], -38, "insert the same record");
      $this -> assertTrue(array_key_exists("errno", $errMsg1));
      $this -> assertTrue(array_key_exists("description", $errMsg1));
      $this -> assertTrue(array_key_exists("detail", $errMsg1));
      if( !globalParameter::isStandalone( self::$db ))
      {
         $this -> assertTrue(array_key_exists("ErrNodes", $errMsg1));
         $this -> assertTrue(array_key_exists("NodeName", $errMsg1["ErrNodes"][0]));
         $this -> assertTrue(array_key_exists("GroupName", $errMsg1["ErrNodes"][0]));
         $this -> assertTrue(array_key_exists("Flag", $errMsg1["ErrNodes"][0]));
         $this -> assertTrue(array_key_exists("ErrInfo", $errMsg1["ErrNodes"][0]));
         $this -> assertTrue(array_key_exists("errno", $errMsg1["ErrNodes"][0]["ErrInfo"]));
         $this -> assertEquals($errMsg1["ErrNodes"][0]["ErrInfo"]["errno"], $errMsg1["errno"]);
         $this -> assertEquals($errMsg1["ErrNodes"][0]["ErrInfo"]["description"], $errMsg1["description"]);
         $this -> assertEquals($errMsg1["ErrNodes"][0]["ErrInfo"]["detail"], $errMsg1["detail"]);
      }
      self::$db -> getCS("NOT_EXIST_CS16657");
      $errMsg2 = self::$db -> getLastErrorMsg();
      $this -> assertEquals( $errMsg2["errno"], -34, "get not exist cl");
   }
   
   public function tearDown()
   {
      $err = self::$db -> dropCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to drop cs, errno=".$err['errno']);
      }
      self::$db->close();
   }
   
}
?>
