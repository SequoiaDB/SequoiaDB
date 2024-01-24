/****************************************************
@description:      update, flag=SDB_FLG_QUERY_PREPARE_MORE(0x00004000)
@testlink cases:   seqDB-13653
@output:     success
@modify list:
        2017-11-29 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DataOperator13653 extends BaseOperator 
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
      $options = null;
      return $this -> commCreateCL( $csName, $clName, $options, true );
   }
   
   function insertRecs( $clDB )
   {
      $recs = array( 'a' => 1 );
      $clDB -> insert( $recs );
   }
   
   function findRecs( $clDB )
   {
      $findRecsArray = array() ;
      $flag = SDB_FLG_QUERY_PREPARE_MORE;
      $cond = '{a:{"$et":1}}';
      $cursor = $clDB -> find($cond, null, null, null, 0, -1, $flag);
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

class TestData13653 extends PHPUnit_Framework_TestCase
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
      
      self::$dbh = new DataOperator13653();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME.'13653';
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 1, $recsArray );
      $expValue = 1;  
      $this -> assertEquals( $expValue, $recsArray[0]['a'] );
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to drop cl in the end.\n";      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      
      self::$endTime = microtime( true );
      echo "\n---End the Test,End time: " . date( "Y-m-d H:i:s", self::$endTime ) . "\n";
      echo "\n---Test 13653 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>