/****************************************************
@description:      remove, cover all data type
@testlink cases:   seqDB-7659
@input:        1 test_createCL
               2 insert, $records: array, cover all data type:
                  $null, $int32, $double, $string, $bool, 
                  $oid, $int64, $date, $binary, $regex, $timestamp, $minKey, $maxKey, 
                  $subobj, $array
               3 find, with condition
               4 remove, with condition cover all data type, $condition: array
                 $hint: null  ----test $hint when test index
               4 dropCL
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DataOperator12 extends BaseOperator 
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
      //insert count: 15
      //nomal data type, count: 7
      $null   = array( 'a' => 0,  'b' => null, 'c' => 'dataType' );
      $int32  = array( 'a' => 1,  'b' => -2147483648, 'c' => 'dataType' );
      $double = array( 'a' => 2,  'b' => -1.7E+308, 'c' => 'dataType' );
      $string = array( 'a' => 3,  'b' => 'test', 'c' => 'dataType' );
      $bool   = array( 'a' => 4,  'b' => true, 'c' => 'dataType' );
      $array  = array( 'a' => 5,  'b' => array( array( 'b1' => 1 ) ), 'c' => 'dataType' );
      //special data type, count: 8
      $oid    = '{"a": 6,  "b": {"$oid": "123abcd00ef12358902300ef"}, "c": "dataType"}';
      $int64  = '{"a": 7,  "b": {"$numberLong": "9223372036854775807"}, "c": "dataType"}';
      $date   = '{"a": 8,  "b": {"$date": "2038-01-18"}, "c": "dataType"}';
      $binary = '{"a": 9,  "b": {"$binary": "aGVsbG8gd29ybGQ=", "$type": "1"}, "c": "dataType"}';
      $regex  = '{"a": 10, "b": {"$regex": "^rg", "$options": "i"}, "c": "dataType"}';
      $timestamp = '{"a": 11, "b": {"$timestamp": "2038-01-18-23.59.59.999999"}, "c": "dataType"}';
      $minKey = '{"a": 12, "b": {"$minKey": 1}, "c": "dataType"}';
      $maxKey = '{"a": 13, "b": {"$maxKey": 1}, "c": "dataType"}';
      $subobj = '{"a": 14, "b": {"subobj": 1111}, "c": "dataType"}';
      
      $recsArray = array( $null, $int32, $double, $string, $bool, $array, 
                          $oid, $int64, $date, $binary, $regex, $timestamp, $minKey, $maxKey, 
                          $subobj );
      
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
      
      return $recsArray;
   }
   
   function removeRecs( $clDB, $recsArray )
   {
      $clDB -> remove( '{a: 0, b: null}', null );
      $clDB -> remove( '{a: 1, b: -2147483648}', null );
      $clDB -> remove( '{a: 2, b: -1.7E+308}', null );
      $clDB -> remove( '{a: 3, b: "test"}', null );
      $clDB -> remove( '{a: 4, b: true}', null );
      $clDB -> remove( '{a: 5, b: [{b1: 1}]}', null );
      $clDB -> remove( '{a: 6, b: {"$oid": "123abcd00ef12358902300ef"} }', null );
      $clDB -> remove( '{a: 7, b: {"$numberLong": "9223372036854775807"} }', null );
      $clDB -> remove( '{a: 8, b: {"$date": "2038-01-18"} }', null );
      $clDB -> remove( '{a: 9, b: {"$binary": "aGVsbG8gd29ybGQ=", "$type": "1"} }', null );
      $clDB -> remove( '{a: 10, b: {$et: {"$regex": "^rg", "$options": "i"} } }', null );
      $clDB -> remove( '{a: 11, b: {"$timestamp": "2038-01-18-23.59.59.999999"} }', null );
      $clDB -> remove( '{a: 12}', null );
      $clDB -> remove( '{a: 13}', null );
      $clDB -> remove( '{a: 14, b: {"subobj": 1111} }', null );
   }
   
   function find( $clDB )
   {
      $cursor = $clDB -> find();
      $tmpArray = array();
      while( $recs = $cursor -> next() )
      {
         array_push( $tmpArray, $recs );
      }
      return $tmpArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestData12 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   private static $rawData;
   private static $beginTime;
   private static $endTime;
   
   public static function setUpBeforeClass()
   {
      date_default_timezone_set("Asia/Shanghai");
      self::$beginTime = microtime( true );
      echo "\n---Begin time: " . date( "Y-m-d H:i:s", self::$beginTime ) ."\n";
   
      self::$dbh = new DataOperator12();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME . '_7659_02';
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$rawData = self::$dbh -> insertRecs( self::$clDB );  //raw data
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_remove()
   {
      echo "\n   Begin to remove records with condtion.\n";
      
      self::$dbh -> removeRecs( self::$clDB, self::$rawData );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_findAfterRemove()
   {
      echo "\n   Begin to find records after remove.\n";
      
      $findRecsArray = self::$dbh -> find( self::$clDB );
      //compare exec result
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare count
      $this -> assertEquals( 0, count( $findRecsArray ) );
   }
   
   function test_dropCL()
   {
      echo "\n---Begin to drop cl in the end.\n";
      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
  
   public static function tearDownAfterClass()
   {
      self::$endTime = microtime( true );
      echo "\n---End the Test,End time: " . date( "Y-m-d H:i:s", self::$endTime ) . "\n";
      echo "\n---Test 7659_02 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
}
?>