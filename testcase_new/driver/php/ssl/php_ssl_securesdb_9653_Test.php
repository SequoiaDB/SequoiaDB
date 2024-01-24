/****************************************************
@description:      ssl, securesdb
@testlink cases:   seqDB-9653
@modify list:
        2017-11-21 xiaoni huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
class sslTest965301 extends PHPUnit_Framework_TestCase
{
   protected static $db;

   public static function setUpBeforeClass()
   {
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();

      // don't use SSL
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address);
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
      // disable SSL
      $err = self::$db -> updateConfig( array('usessl' => true) );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to enable SSL using updateConfig, errno=".$err['errno']);
      }
      self::$db -> close();

      /*  SEQUOIADBMAINSTREAM-8018
      // use SSL
      self::$db = new SecureSdb($address);
      $err = self::$db -> getLastErrorMsg();
      if( $err['errno'] != 0 ) {
         throw new Exception("failed to connect db using SSL, errno=".$err['errno']);
      }
      */
   }

   public function test_ssl()
   {
   }

   public static function tearDownAfterClass()
   {
      $err = self::$db->close();
   }

};
?>