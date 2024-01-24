/****************************************************
@description:      rename cl, check new clName
@testlink cases:   seqDB-16556
@input:        rename cl
@output:     success
@modify list:
        2018-11-13 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class Rename16556 extends PHPUnit_Framework_TestCase
{
   private static $csName = 'cs16556';
   private static $clName = 'cl16556';
   private static $cs;
   private static $db;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );                     
                           
      self::$cs = self::$db -> selectCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$cs -> selectCL( self::$clName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
   }
   
   function test()
   {
      echo "\n---Begin to rename cs.\n";
      
      //use null cl name
      self::$cs -> renameCL( self::$clName, null );
      $this -> assertEquals( -6, self::$db -> getError()['errno'] );
      self::$db -> getCS( self::$csName ) -> getCL( self::$clName ); 
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      //use "" cl name
      self::$cs -> renameCL( self::$clName, '' );
      $this -> assertEquals( -6, self::$db -> getError()['errno'] );
      self::$db -> getCS( self::$csName ) -> getCL( self::$clName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

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
