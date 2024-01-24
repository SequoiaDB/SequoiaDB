/****************************************************
@description:      test rename cs success
@testlink cases:   seqDB-16549
@input:        rename cs
@output:     success
@modify list:
        2018-11-13 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class Rename16549 extends PHPUnit_Framework_TestCase
{
   private static $oldCSName = 'cs16549old';
   private static $newCSName = 'cs16549new';
   private static $clName = 'cl16549';
   private static $cs;
   private static $db;
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );                     
                           
      self::$cs = self::$db -> selectCS( self::$oldCSName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$cs -> selectCL( self::$clName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
   }
   
   function test()
   {
      echo "\n---Begin to rename cs.\n";
      
      self::$db -> renameCS( self::$oldCSName, self::$newCSName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      self::$db -> getCS( self::$oldCSName );
      $this -> assertEquals( -34, self::$db -> getError()['errno'] );
      
      $csSnapCur = self::$db -> snapshot( SDB_SNAP_COLLECTIONSPACES, array( 'Name' => self::$newCSName) );
      if( empty( $csSnapCur ) )
      {  
         throw new Exception( self::$newCSName . " is not exist, check snapshot error");
      }
      while( $record = $csSnapCur -> next())
      {  
         $clArr = $record["Collection"];
         $actCSName = explode( ".", $clArr[0]["Name"] )[0];
         $this -> assertEquals( $actCSName, self::$newCSName);
      }
   }
   
   public function tearDown()
   {
      $err = self::$db -> dropCS( self::$newCSName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      echo "\n---End of the test.\n";
      self::$db->close();
   }
}
?>
