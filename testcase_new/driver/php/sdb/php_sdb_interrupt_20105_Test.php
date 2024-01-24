/****************************************************
@description:      interrupt接口测试
@testlink cases:   seqDB-20105
@modify list:
        2019-10-26 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class interrupt20105 extends PHPUnit_Framework_TestCase
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
      echo "\n---Begin to test interrupt.\n";
      
      $contextCur = self::$db -> snapshot( SDB_SNAP_CONTEXTS );
      $sessionCur = self::$db -> snapshot( SDB_SNAP_SESSIONS );
      
      self::$db -> interrupt();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $rc = $contextCur -> next();
      $this -> assertEquals( -31, self::$db -> getError()['errno'] );
      
      $rc = $sessionCur -> next();
      $this -> assertEquals( -31, self::$db -> getError()['errno'] );
   }
   
   public function tearDown()
   {
      echo "\n---check interrupt complete.\n";
      self::$db->close();
   }
}