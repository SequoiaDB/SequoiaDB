/****************************************************
@description:      rename cs, old cs name not exist, new cs name equal, new cs name exist
@testlink cases:   seqDB-16550,seqDB-16551
@input:        rename cs
@output:     success
@modify list:
        2018-11-13 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class Rename16550_16551 extends PHPUnit_Framework_TestCase
{
   private static $csName1 = 'cs16550_1';
   private static $csName2 = 'cs16550_2';
   private static $clName = 'cl16550';
   private static $cs;
   private static $db;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );                     
                           
      self::$cs = self::$db -> selectCS( self::$csName1 );
      self::$db -> selectCS( self::$csName2 );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$cs -> selectCL( self::$clName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
   }
   
   function test()
   {
      echo "\n---Begin to rename cs.\n";
      
      self::$db -> renameCS( 'csNameNotExist', self::$csName1 );
      $this -> assertEquals( -34, self::$db -> getError()['errno'] );
      
      self::$db -> renameCS( self::$csName1, self::$csName1 );
      $this -> assertEquals( -33, self::$db -> getError()['errno'] );
      
      
      self::$db -> renameCS( self::$csName1, self::$csName2 );
      $this -> assertEquals( -33, self::$db -> getError()['errno'] );
      
   }
   
   public function tearDown()
   {
      $err = self::$db -> dropCS( self::$csName1 );
      if ( $err['errno'] != 0 )
      {
         throw new Exception('failed to drop cs, errno='.$err['errno']);
      }
      $err = self::$db -> dropCS( self::$csName2 );
      if ( $err['errno'] != 0 )
      {
         throw new Exception('failed to drop cs, errno='.$err['errno']);
      }
      echo "\n---End of the test.\n";
      self::$db->close();
   }
   
}
?>
