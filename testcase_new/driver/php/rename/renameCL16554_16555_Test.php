/****************************************************
@description:      rename cl, old cl name not exist, new cl name equal, new cl name exist
@testlink cases:   seqDB-16554 seqDB-16555
@input:        rename cl
@output:     success
@modify list:
        2018-11-13 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class Rename16554_16555 extends PHPUnit_Framework_TestCase
{
   private static $csName = 'cs16554';
   private static $clName1 = 'cl16554_1';
   private static $clName2 = 'cl16554_2';
   private static $cs;
   private static $cl;
   private static $db;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
                           
      self::$cs = self::$db -> selectCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
      self::$cs -> selectCL( self::$clName1 );
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
      self::$cs -> selectCL( self::$clName2 );
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
      
   }
   
   function test()
   {
      echo "\n---Begin to rename cs.\n";
      
      self::$cs -> renameCL( 'clNameNotExist', self::$clName1 );
      $this -> assertEquals( -23, self::$db -> getError()['errno'] );
      
      self::$cs -> renameCL( self::$clName1, self::$clName1 );
      $this -> assertEquals( -22, self::$db -> getError()['errno'] );
      
      self::$cs -> renameCL( self::$clName1, self::$clName2 );
      $this -> assertEquals( -22, self::$db -> getError()['errno'] );
      
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
