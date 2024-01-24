/****************************************************
@description:      syncDB
@testlink cases:   seqDB-13654
@modify list:
        2017-11-21 xiaoni huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
class syncDB13654 extends PHPUnit_Framework_TestCase
{
   protected static $db ;
   protected static $clDB ;
   protected static $csName  = 'cs13654';
   protected static $clName  = 'cl';

   public static function setUpBeforeClass()
   {
      // connect
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '');
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
      
      // create cs
      $csDB = self::$db -> selectCS( self::$csName, null );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".self::$db -> getError()['errno']);
      }
      
      // create cl
      $option = '{ReplSize:0}';
      self::$clDB = $csDB -> selectCL( self::$clName, $option );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
   }

   public function test_syncDB()
   {  
      echo "\n---Begin to syncDB.\n";
      // insert records
      $err = self::$clDB -> insert('{a:1}');
      $this -> assertEquals( 0, $err['errno'] );
      
      // syncDB
      $err = self::$db -> syncDB();
      $this -> assertEquals( 0, $err['errno'] );
      
      // check
      $cnt = self::$clDB -> count('{a:1}');
      $this -> assertEquals( 1, $cnt );
   }

   public function test_syncDBArray()
   {  
      echo "\n---Begin to syncDB by array.\n";
      // insert records
      $err = self::$clDB -> insert('{arr:1}');
      $this -> assertEquals( 0, $err['errno'] );
      
      // syncDB, array
      $err = self::$db -> syncDB( array( 'CollectionSpace' => self::$csName ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      // check
      $cnt = self::$clDB -> count('{arr:1}');
      $this -> assertEquals( 1, $cnt );
   }
   
   public function test_syncDBString()
   {  
      echo "\n---Begin to syncDB by string.\n";
      // insert records
      $err = self::$clDB -> insert('{str:1}');
      $this -> assertEquals( 0, $err['errno'] );
      
      // syncDB, string
      $err = self::$db -> syncDB('{CollectionSpace:"'.self::$csName.'"}');
      $this -> assertEquals( 0, $err['errno'] );
      
      // check
      $cnt = self::$clDB -> count('{str:1}');
      $this -> assertEquals( 1, $cnt );
   }
   
   public static function tearDownAfterClass()
   {
      $err = self::$db -> dropCS( self::$csName ); 
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to drop cs, errno=".$err['errno']);
      }
      
      $err = self::$db->close();
   }
};
?>