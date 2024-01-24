/****************************************************
@description:      ssl, sequoiadb
@testlink cases:   seqDB-9652
@modify list:
        2017-11-21 xiaoni huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
class sslTest965202 extends PHPUnit_Framework_TestCase
{
   protected static $db ;
   protected static $csName  = 'cs9652_seq';
   protected static $clName  = 'cl';

   public static function setUpBeforeClass()
   {
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();

      // don't use SSL
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '', false);
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
      // enable SSL
      $err = self::$db -> updateConfig( array('usessl' => true) );
      if ( $err['errno'] != 0 )
      {
          throw new Exception("failed to enable SSL using updateConfig, errno=".$err['errno']);
      }
      self::$db -> close();

      // use SSL
      $err = self::$db -> connect($address, '', '', true);
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db using SSL, errno=".$err['errno']);
      }
   }

   public function test_ssl()
   {
      // create cs
      self::$db -> createCS( self::$csName, null );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      // get cs
      $csDB = self::$db -> getCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      // create cl
      $csDB -> createCL( self::$clName, null );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      // drop cs
      self::$db -> dropCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }

   public static function tearDownAfterClass()
   {
      $err = self::$db->close();
   }

};
?>