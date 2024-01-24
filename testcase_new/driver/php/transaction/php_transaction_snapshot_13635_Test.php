/****************************************************
@description:      transaction, snapshot or list
@testlink cases:   seqDB-13635
@modify list:
        2017-11-28 xiaoni huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
class snapshotTransactionTest13635 extends PHPUnit_Framework_TestCase
{
   protected static $db ;
   protected static $clDB ;
   protected static $csName  = 'cs13635_snap';
   protected static $clName  = 'cl';

   public static function setUpBeforeClass()
   {
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '');
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
            
      // create cs
      $err = self::$db -> createCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".$err['errno']);
      }
      
      // get cs
      $csDB = self::$db -> getCS( self::$csName );
      $err  = self::$db -> getError();
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to get cs, errno=".$err['errno']);
      }      
      
      // create cl
      self::$clDB = $csDB -> selectCL( self::$clName );
      $err  = self::$db -> getError();
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".$err['errno']);
      }
   }

   public function test_snapshotTrans()
   {
      self::$db -> transactionBegin();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      // insert
      self::$clDB -> insert( '{a:1}' );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      // snapshot
      $cursor = self::$db -> snapshot( SDB_SNAP_TRANSACTION );
      if ( empty( $cursor) ) {
         $this -> assertTrue( false, true,  'return is empty'); 
      }      
      
      $cursor = self::$db -> snapshot( SDB_SNAP_TRANSACTION_CURRENT );
      if ( empty( $cursor) ) {
         $this -> assertTrue( false, true,  'return is empty'); 
      }
      
      self::$db -> transactionRollback();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   public static function tearDownAfterClass()
   {
      self::$db -> dropCS(self::$csName);      
      self::$db->close();
   }
   
};
?>