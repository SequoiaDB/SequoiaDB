/****************************************************
@description:      update shardingKey
@testlink cases:   seqDB-12631
@modify list:
        2017-11-28 xiaoni huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class UpdateShardingKey12631 extends BaseOperator 
{  
   public function __construct()
   {
      parent::__construct();
   }
   
   function getErrno()
   {
      $this -> err = $this -> db -> getError();
      return $this -> err['errno'];
   }
   
   function createCL( $csName, $clName )
   {
      $options = array( 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'range', 
                        'ReplSize' => 0 );
      return $this -> commCreateCL( $csName, $clName, $options, true );
   }
   
   function insertRecs( $clDB )
   {
      $recs = array( 'a' => 1, 'b' => 1 );
      $clDB -> insert( $recs );
   }
   
   function updateRecs( $clDB )
   {
      $rule = array( '$inc' => array( 'a' => 1, 'b' => 1 ) );
      $clDB -> update( $rule, null, null, SDB_FLG_UPDATE_KEEP_SHARDINGKEY );
   } 
   
   function findRecs( $clDB )
   {
      $findRecsArray = array();
      $cursor = $clDB -> find();
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
      return $findRecsArray;
   } 
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestUpdateShardingKey12631 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   private static $beginTime;
   private static $endTime;
   
   public static function setUpBeforeClass()
   {
      date_default_timezone_set("Asia/Shanghai");
      self::$beginTime = microtime( true );
      echo "\n---Begin time: " . date( "Y-m-d H:i:s", self::$beginTime ) ."\n";
   
      self::$dbh = new UpdateShardingKey12631();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "\n---Begin to ready parameter.\n";
         self::$csName = self::$dbh -> COMMCSNAME;
         self::$clName = self::$dbh -> COMMCLNAME.'12631';
         
         echo "\n---Begin to drop cl in the begin.\n";
         self::$dbh -> dropCL( self::$csName, self::$clName, true );
         
         echo "\n---Begin to create cl.\n";
         self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
      }
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_update()
   {
      echo "\n---Begin to update records.\n";
      
      self::$dbh -> updateRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -178, $errno );
   }
   /*
   function test_find()
   {
      echo "\n---Begin to find records after update.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 1, $recsArray );
      $expValue = 2;  
      $this -> assertEquals( $expValue, $recsArray[0]['a'] );
      $this -> assertEquals( $expValue, $recsArray[0]['b'] );
   }*/
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to drop cl in the end.\n";      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      
      self::$endTime = microtime( true );
      echo "\n---End the Test,End time: " . date( "Y-m-d H:i:s", self::$endTime ) . "\n";
      echo "\n---Test 12631 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>