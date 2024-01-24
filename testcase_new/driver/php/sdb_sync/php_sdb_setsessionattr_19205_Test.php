/****************************************************
@description:      清空会话缓存中的事务配置
@testlink cases:   seqDB-19205
@modify list:
        2019-10-18 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class SetSessionAttr19205 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $defaultAttr;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      self::$defaultAttr = array( "PreferedInstance" => "M",
                                  "PreferredInstance" => "M",
                                  "PreferedInstanceMode" => "random",
                                  "PreferredInstanceMode" => "random",
                                  "PreferedStrict" => false,
                                  "PreferredStrict" => false,
                                  "PreferedPeriod" => 60,
                                  "PreferredPeriod" => 60,
                                  "Timeout" => new SequoiaINT64('-1'),
                                  "TransIsolation" => 0,
                                  "TransTimeout" => 60,
                                  "TransUseRBS" => true,
                                  "TransLockWait" => false,
                                  "TransAutoCommit" => false,
                                  "TransAutoRollback" => true,
                                  "TransRCCount" => true,
                                  "Source" => "");
   }
   
   function test()
   {
      echo "\n---Begin to get sessionAttr.\n";

      try
      {
         $sessionAttr = self::$db -> getSessionAttr();
         if( !is_object( $sessionAttr["Timeout"] ) )
         {
            self::$defaultAttr["Timeout"] = -1;
         }
         if( !globalParameter::compareArray( $sessionAttr, self::$defaultAttr ) )
         {
            throw new Exception("chech attr value error: \nexpAttr: " . json_encode(self::$defaultAttr) . "\nactAttr: " . json_encode($sessionAttr));
         }
         
         self::$db -> setSessionAttr( array( 'TransIsolation' => 2 ) );
         $this -> assertEquals( 0, self::$db -> getError()['errno'] );
         
         self::$defaultAttr['TransIsolation'] = 2;
         $sessionAttr = self::$db -> getSessionAttr();
         if( !globalParameter::compareArray( $sessionAttr, self::$defaultAttr ) )
         {
            throw new Exception("chech attr value error: \nexpAttr: " . json_encode(self::$defaultAttr) . "\nactAttr: " . json_encode($sessionAttr));
         }
         
         self::$db -> updateConfig( array('transisolation' => 0, 'transactiontimeout' => 120) ) ;
         $this -> assertEquals( 0, self::$db -> getError()['errno'] );
         
         self::$db -> setSessionAttr(array());
         $this -> assertEquals( 0, self::$db -> getError()['errno'] );
         
         self::$defaultAttr['TransTimeout'] = 120;
         $sessionAttr = self::$db -> getSessionAttr();
         if( !globalParameter::compareArray( $sessionAttr, self::$defaultAttr ) )
         {
            throw new Exception("chech attr value error: \nexpAttr: " . json_encode(self::$defaultAttr) . "\nactAttr: " . json_encode($sessionAttr));
         }
         
         $sessionAttr = self::$db -> getSessionAttr(false);
         if( !globalParameter::compareArray( $sessionAttr, self::$defaultAttr ) )
         {
            throw new Exception("chech attr value error: \nexpAttr: " . json_encode(self::$defaultAttr) . "\nactAttr: " . json_encode($sessionAttr));
         }
      }
      catch(Exception $e)
      {
         //用例执行失败不会执行tearDown，需要catch后执行环境恢复
         self::$db -> updateConfig( array('transisolation' => 0, 'transactiontimeout' => 60) ) ;
         $this -> assertEquals( 0, self::$db -> getError()['errno'] );
         throw $e;
      }
   }
   
   public function tearDown()
   {
      echo "\n---test complete.\n";
      
      //用例执行结束，恢复环境
      self::$db -> updateConfig( array('transisolation' => 0, 'transactiontimeout' => 60) ) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$db->close();
   }
}
?>
