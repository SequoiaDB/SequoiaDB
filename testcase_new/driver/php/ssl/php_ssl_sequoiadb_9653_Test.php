/****************************************************
@description:      ssl, sequoiadb
@testlink cases:   seqDB-9652
@modify list:
        2017-11-21 xiaoni huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
class sslTest965302 extends PHPUnit_Framework_TestCase
{
   protected static $db ;

   public function test_ssl()
   {
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();

      // don't use SSL
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '', false);
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
      // disable SSL
      $err = self::$db -> updateConfig( array('usessl' => false) );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to enable SSL using updateConfig, errno=".$err['errno']);
      }
      self::$db -> close();

      // use SSL
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '', true);
      if ( $err['errno'] != -15 )
      {
         throw new Exception("Failed to use SSL connection when SSL is disabled, errno=".$err['errno']);
      }

   }

};
?>