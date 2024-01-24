/****************************************************
@description:      interruptOperation接口测试
@testlink cases:   seqDB-20213
@modify list:
        2019-10-26 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class interruptOperation20213 extends PHPUnit_Framework_TestCase
{
   private static $db;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   function test()
   {
      echo "\n---Begin to test interruptOperation.\n";
      
      $sessionCur = self::$db -> snapshot( SDB_SNAP_SESSIONS );
      
      self::$db -> interruptOperation();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      while($rc = $sessionCur -> next())
      {
         $str = $rc;
         $this -> assertEquals( 0, self::$db -> getError()['errno'] );   
      }
      
   }
   
   public function tearDown()
   {
      echo "\n---check interruptOperation complete.\n";
      self::$db->close();
   }
}