/****************************************************
@description:      test rename cl success
@testlink cases:   seqDB-16553
@input:        rename cl
@output:     success
@modify list:
        2018-11-13 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class Rename16553 extends PHPUnit_Framework_TestCase
{
   private static $csName = 'cs16553';
   private static $oldCLName = 'cl16549old';
   private static $newCLName = 'cl16549new';
   private static $cs;
   private static $db;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
                           
      self::$cs = self::$db -> selectCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
      self::$cs -> selectCL( self::$oldCLName );
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
      
   }
   
   function test()
   {
      echo "\n---Begin to rename cs.\n";
      
      self::$cs -> renameCL( self::$oldCLName, self::$newCLName );
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
      
      self::$db -> getCS( self::$csName ) -> getCL( self::$oldCLName );
      $this -> assertEquals( -23, self::$db -> getError()['errno']);

      self::$db -> getCS( self::$csName ) -> getCL( self::$newCLName );
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
   }
   
   public function tearDown()
   {
      $err = self::$db -> dropCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception('failed to drop cs, errno='.$err['errno']);
      }
      echo "\n---End of the test.\n";
      self::$db->close();
   }
   
}
?>
