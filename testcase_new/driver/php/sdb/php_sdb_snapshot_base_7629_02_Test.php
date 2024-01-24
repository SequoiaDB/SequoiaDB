/****************************************************
@description:      snapshot, base case
@testlink cases:   seqDB-7629
@input:      1 createCS
             2 snapshot, cover all type
             3 dropCS
@output:     success
@modify list:
        2018-3-10 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/ReplicaGroupMgr.php';
include_once dirname(__FILE__).'/../global.php';
class snapshot762902 extends PHPUnit_Framework_TestCase
{
   protected static $db ;
   private static $groupMgr; 
   protected static $clDB ;
   protected static $csName = 'cs7629_02';
   protected static $clName = 'cl';
   private static $skipTestCase = false;  

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
      
      // judge groupNum
      self::$groupMgr = new ReplicaGroupMgr(self::$db);
      if ( self::$groupMgr -> getError() != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return;
      }
      
      if (self::$groupMgr -> getDataGroupNum() < 1)
      {
         self::$skipTestCase = true ;
         return;
      }
      
      // create cs
      $csDB = self::$db -> selectCS( self::$csName, null );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".self::$db -> getError()['errno']);
      }
      
      // create cl
      $option = array( 'ReplSize' => 0 );
      self::$clDB = $csDB -> selectCL( self::$clName, $option );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // insert
      self::$clDB -> insert('{a:1}');
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to in, errno=".self::$db -> getError()['errno']);
      }
   }
   
   public function setUp()
   {
      if( self::$skipTestCase === true )
      {
         $this -> markTestSkipped( "init failed" );
      }
   }

   public function test_snapshotContexts()
   {
      $type = SDB_SNAP_CONTEXTS;
      echo "\n---Begin to exec snapshot[SDB_SNAP_CONTEXTS, ".$type."].\n";
      $this -> assertEquals( 0, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }

      $record = $cursor -> current();
      if( is_object( $record["SessionID"] ) )
      {
         $this -> assertNotEmpty( $record["SessionID"] -> __toString() );
         $this -> assertNotEmpty( $record["Contexts"][0]["ContextID"] -> __toString() );
      }
      else
      {
         $this -> assertNotEmpty( $record["SessionID"] );
         $this -> assertNotEmpty( $record["Contexts"][0]["ContextID"] );
      }
   }

   public function test_snapshotContextsCurrent()
   {
      $type = SDB_SNAP_CONTEXTS_CURRENT;
      echo "\n---Begin to exec snapshot[SDB_SNAP_CONTEXTS_CURRENT, ".$type."].\n";
      $this -> assertEquals( 1, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $record = $cursor -> current();
      $this -> assertEquals( "DUMP", $record["Contexts"][0]["Type"] );
   }

   public function test_snapshotSessions()
   {
      $type = SDB_SNAP_SESSIONS;
      echo "\n---Begin to exec snapshot[SDB_SNAP_SESSIONS, ".$type."].\n";
      $this -> assertEquals( 2, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $record = $cursor -> current() ;
      $this -> assertTrue( isset( $record["SessionID"] ) );
      $this -> assertTrue( isset( $record["TID"] ) );
   }

   public function test_snapshotSessionsCurrent()
   {
      $type = SDB_SNAP_SESSIONS_CURRENT;
      echo "\n---Begin to exec snapshot[SDB_SNAP_SESSIONS_CURRENT, ".$type."].\n";
      $this -> assertEquals( 3, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $record = $cursor -> current();
      $this -> assertTrue( isset( $record["SessionID"] ) );
      $this -> assertTrue( isset( $record["TID"] ) );
   }

   public function test_snapshotCullections()
   {
      $type = SDB_SNAP_COLLECTIONS;
      echo "\n---Begin to exec snapshot[SDB_SNAP_COLLECTIONS, ".$type."].\n";
      $this -> assertEquals( 4, $type );
      
      $type = SDB_SNAP_COLLECTION;
      $this -> assertEquals( 4, $type );
      
      $type = SDB_SNAP_COLLECTIONS;
      $cond = array( 'Name' => self::$csName.'.'.self::$clName );
      $cursor = self::$db -> snapshot( $type, $cond );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $this -> assertEquals( self::$csName.'.'.self::$clName, $cursor -> current()["Name"] );
      $this -> assertEquals( "Normal", $cursor -> current()["Details"][0]["Group"][0]["Status"] );
   }

   public function test_snapshotCullectionsSpaces()
   {
      $type = SDB_SNAP_COLLECTIONSPACES;
      echo "\n---Begin to exec snapshot[SDB_SNAP_COLLECTIONSPACES, ".$type."].\n";
      $this -> assertEquals( 5, $type );
      
      $type = SDB_SNAP_COLLECTIONSPACE;
      $this -> assertEquals( 5, $type );
      
      $type = SDB_SNAP_COLLECTIONSPACES;
      $cond = array( 'Name' => self::$csName );
      $cursor = self::$db -> snapshot( $type, $cond );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $record = $cursor -> current();
      $this -> assertEquals( self::$csName, $record["Name"] );
      $this -> assertTrue( isset( $record["PageSize"] ) );
   }

   public function test_snapshotDatabase()
   {
      $type = SDB_SNAP_DATABASE;
      echo "\n---Begin to exec snapshot[SDB_SNAP_DATABASE, ".$type."].\n";
      $this -> assertEquals( 6, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $record = $cursor -> current();
      $this -> assertTrue( isset( $record["TotalNumConnects"] ) );
      $this -> assertTrue( isset( $record["TotalDataRead"] ) );
   }

   public function test_snapshotSystem()
   {
      $type = SDB_SNAP_SYSTEM;
      echo "\n---Begin to exec snapshot[SDB_SNAP_SYSTEM, ".$type."].\n";
      $this -> assertEquals( 7, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $record = $cursor -> current();
      $this -> assertGreaterThan( 0, $record["CPU"]["User"] );
      $this -> assertGreaterThan( 0, $record["Memory"]["TotalRAM"] );
   }

   public function test_snapshotCatalog()
   {
      $type = SDB_SNAP_CATALOG;
      echo "\n---Begin to exec snapshot[SDB_SNAP_CATALOG, ".$type."].\n";
      $this -> assertEquals( 8, $type );
      
      $type = SDB_SNAP_CATA;
      $this -> assertEquals( 8, $type );
      
      $type = SDB_SNAP_CATALOG;
      $cond = array( 'Name' => self::$csName.'.'.self::$clName );
      $cursor = self::$db -> snapshot( $type, $cond );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $this -> assertEquals( self::$csName.'.'.self::$clName, $cursor -> current()["Name"] );
      $record = $cursor -> current();
      $this -> assertTrue( isset( $record["CataInfo"] ) );
      //var_dump( $cursor -> next() );
   }

   public function test_snapshotTransactions()
   {
      $type = SDB_SNAP_TRANSACTIONS;
      echo "\n---Begin to exec snapshot[SDB_SNAP_TRANSACTIONS, ".$type."].\n";
      $this -> assertEquals( 9, $type );
      
      $type = SDB_SNAP_TRANSACTION;
      $this -> assertEquals( 9, $type );
      
      $type = SDB_SNAP_TRANSACTIONS;
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      //var_dump( $cursor -> next() );
   }

   public function test_snapshotTransactionsCurrent()
   {  
      $type = SDB_SNAP_TRANSACTIONS_CURRENT;
      echo "\n---Begin to exec snapshot[SDB_SNAP_TRANSACTIONS_CURRENT, ".$type."].\n";
      $this -> assertEquals( 10, $type );
      
      $type = SDB_SNAP_TRANSACTION_CURRENT;
      $this -> assertEquals( 10, $type );
      
      $type = SDB_SNAP_TRANSACTIONS_CURRENT;
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
   }

   public function test_snapshotAccessplans()
   {
      $type = SDB_SNAP_ACCESSPLANS;
      echo "\n---Begin to exec snapshot[SDB_SNAP_ACCESSPLANS, ".$type."].\n";
      $this -> assertEquals( 11, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $this -> assertFalse( empty($cursor), "results is empty." );
      
      $record = $cursor -> current();
      $this -> assertTrue( isset( $record["CacheLevel"] ) );
      $this -> assertTrue( isset( $record["Query"] ) );
   }

   public function test_snapshotHealth()
   {
      $type = SDB_SNAP_HEALTH;
      echo "\n---Begin to exec snapshot[SDB_SNAP_HEALTH, ".$type."].\n";
      $this -> assertEquals( 12, $type );
      
      $cursor = self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      if ( empty($cursor) ) {
         $this -> assertFalse( true, "results is empty." );
      }
      $record = $cursor -> current();
      $this -> assertTrue( isset( $record["IsPrimary"] ) );
      $this -> assertTrue( isset( $record["ServiceStatus"] ) );
      $this -> assertTrue( isset( $record["Status"] ) );
      $this -> assertTrue( isset( $record["BeginLSN"] ) );
   }
   
   public function test_snapshotQueries()
   {
      $type = SDB_SNAP_QUERIES	;
      echo "\n---Begin to exec snapshot[SDB_SNAP_QUERIES	, ".$type."].\n";
      $this -> assertEquals( 18, $type );
      
      self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   public function test_snapshotLatchWaits()
   {
      $type = SDB_SNAP_LATCHWAITS;
      echo "\n---Begin to exec snapshot[SDB_SNAP_LATCHWAITS, ".$type."].\n";
      $this -> assertEquals( 19, $type );
      
      self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   public function test_snapshotLockWaits()
   {
      $type = SDB_SNAP_LOCKWAITS;
      echo "\n---Begin to exec snapshot[SDB_SNAP_LOCKWAITS, ".$type."].\n";
      $this -> assertEquals( 20, $type );
      
      self::$db -> snapshot( $type );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   public static function tearDownAfterClass()
   {
      if ( self::$skipTestCase == false )
      {
         echo "\n---Begin to dropCS in the end.\n";
         $err = self::$db -> dropCS( self::$csName );
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
      }
      
      $err = self::$db->close();
   }
};
?>