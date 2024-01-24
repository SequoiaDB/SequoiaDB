/****************************************************
@description:      findAndUpdate, cover all data type
@testlink cases:   seqDB-7664
@input:        1 test_createCL
               2 insert, cover all data type:
                  $null, $int32, $double, $string, $bool, $subobj, $array, 
                  $oid, $int64, $date, $binary, $regex, $timestamp, $minKey, $maxKey
               3 findAndupdate, with condition cover all data type, 
                     $rule/$condition/$selector/$orderby/$hint: string
                     returnNew: true
               5 count, cover required parameter, $condition: string
               6 dropCL
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DataOperator07 extends BaseOperator 
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
   
   function createIndex( $clDB, $idxName )
   {
      $indexDef = array( 'a' => 1 );
      $clDB -> createIndex( $indexDef, $idxName );
   }
   
   function insertRecs( $clDB )
   {
      //insert count: 18
      //nomal data type, count: 7
      $null   = array( 'a' => 0,  'b' => null, 'c' => 'null' );
      $int32  = array( 'a' => 1,  'b' => -2147483648, 'c' => 'int32' );
      $double = array( 'a' => 2,  'b' => -1.7E+308, 'c' => 'double' );
      $string = array( 'a' => 3,  'b' => 'test', 'c' => 'string' );
      $bool   = array( 'a' => 4,  'b' => true, 'c' => 'bool' );
      $array  = array( 'a' => 5,  'b' => array( array( 'b1' => 1 ) ), 'c' => 'array' );
      //special data type, count: 8
      $oid    = array( 'a' => 6,  'b' => new SequoiaID( '123abcd00ef12358902300ef' ), 'c' => 'oid' );
      $int64  = array( 'a' => 7,  'b' => new SequoiaINT64( "-9223372036854775808" ), 'c' => 'int64' );
      $date   = array( 'a' => 8,  'b' => new SequoiaDate( '1991-11-27' ), 'c' => 'date' );
      $binary = array( 'a' => 9,  'b' => new SequoiaBinary( 'aGVsbG8gd29ybGQ=', '1' ), 'c' => 'binary' );
      $regex  = array( 'a' => 10, 'b' => new SequoiaRegex( '^rg', 'i' ), 'c' => 'regex' );
      $timestamp = array( 'a' => 11, 'b' => new SequoiaTimestamp( '1901-12-15-00.00.00.000000' ), 'c' => 'timestamp' );
      $minKey = array( 'a' => 12, 'b' => new SequoiaMinKey(), 'c' => 'minKey' );
      $maxKey = array( 'a' => 13, 'b' => new SequoiaMaxKey(), 'c' => 'maxKey' );
      $subobj = array( 'a' => 14, 'b' => array( 'subobj' => "111" ), 'c' => 'subobj' );
      //others for find by condition, count: 3
      $tmp1 = array( 'a' => -1, 'c' => 'tmp1' );
      $tmp2 = array( 'a' => 15, 'c' => 'tmp2' );
      $tmp3 = array( 'a' => 16 );
      
      $recsArray = array( $null, $int32, $double, $string, $bool, $array, 
                          $oid, $int64, $date, $binary, $regex, $timestamp, $minKey, $maxKey, 
                          $subobj, 
                          $tmp1, $tmp2, $tmp3 );
      
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
      
      return $recsArray;
   }
   
   function findAndUpdateRecs( $clDB, $recsArray, $idxName )
   {
      $rule = '{$set: {b:"updateTest"} }';
      //$condition, see below
      $selector  = '{a:"", b:"", c:"", d:"hello"}';
      $orderby   = json_encode( array( 'a' => 1 ) );
      $hint      = '{"": "'. $idxName .'"}';
      $numToSkip = 0;
      $numToReturn = 15;
      $flag = 0;
      $returnNew = true;
     
      $updateReturnArray = array();
      for( $i = 0; $i < count( $recsArray ) - 3 ; $i++ )  //filter 3 records after find
      {
         if( $i === 0 )     { $condition = '{b:null}'; }
         else if( $i === 1 ){ $condition = '{b:-2147483648}'; }
         else if( $i === 2 ){ $condition = '{b:-1.7E+308}'; }
         else if( $i === 3 ){ $condition = '{b:"test"}'; }
         else if( $i === 4 ){ $condition = '{b: true}'; }
         else if( $i === 5 ){ $condition = '{b:[{b1:1}]}'; }
         else if( $i === 6 ){ $condition = '{b:{$oid:"123abcd00ef12358902300ef"}}'; }
         else if( $i === 7 ){ $condition = '{b: -9223372036854775808}'; }
         else if( $i === 8 ){ $condition = '{b:{$date:"1991-11-27"}}'; }
         else if( $i === 9 ){ $condition = '{b:{$binary:"aGVsbG8gd29ybGQ=", $type:"1"}}'; }
         else if( $i === 10 ){ $condition = '{b:{$et:{$regex:"^rg", $options:"i"}}}'; }
         else if( $i === 11 ){ $condition = '{b:{$timestamp:"1901-12-15-00.00.00.000000"}}'; }
         else if( $i === 12 ){ $condition = '{a:12}'; }  //can not match minkey/maxkey, same sdb shell
         else if( $i === 13 ){ $condition = '{a:13}'; }
         else if( $i === 14 ){ $condition = '{b:{subobj:"111"}}'; }
         
         $cursor = $clDB -> findAndUpdate( $rule, $condition, $selector, $orderby, $hint, 
                                           $numToSkip, $numToReturn, $flag, $returnNew );
         $errno = $this -> getErrno();
         if( $errno !== 0 )
         {
            echo "\nFailed to findAndUpdate. Errno: ". $errno ."\n";
         }
         
         while( $tmpInfo = $cursor -> next() )
         {
            array_push( $updateReturnArray, $tmpInfo );
         }
      }
      
      return $updateReturnArray;
   }
   
   function countRecs( $clDB )
   {
      $condition = json_encode( array( 'b' => 'updateTest' ) );
      //$hint: default null
      return $clDB -> count( $condition );
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestData07 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $idxName;
   private static $clDB;
   private static $rawData;
   private static $beginTime;
   private static $endTime;
   
   public static function setUpBeforeClass()
   {
      date_default_timezone_set("Asia/Shanghai");
      self::$beginTime = microtime( true );
      echo "\n---Begin time: " . date( "Y-m-d H:i:s", self::$beginTime ) ."\n";
   
      self::$dbh = new DataOperator07();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName  = self::$dbh -> COMMCLNAME . '_7664_02';
      self::$idxName = self::$dbh -> CHANGEDPREFIX .'_index';
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_createIndex()
   {
      echo "\n---Begin to create index.\n";
      
      self::$rawData = self::$dbh -> createIndex( self::$clDB, self::$idxName );  //raw data
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$rawData = self::$dbh -> insertRecs( self::$clDB );  //raw data
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_findAndUpdate()
   {
      echo "\n---Begin to findAndUpdata.\n";
      
      $updateReturnArray = self::$dbh -> findAndUpdateRecs( self::$clDB, self::$rawData, self::$idxName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare return count
      $this -> assertCount( 15, $updateReturnArray );
      
      //compare for $selector
      $this -> assertEquals( 'hello', $updateReturnArray[0]['d'] );
      $this -> assertEquals( 'hello', $updateReturnArray[14]['d'] );
      
      //compare for $numToReturn
      $this -> assertEquals( 'updateTest', $updateReturnArray[0]['b'] );
      $this -> assertEquals( 'updateTest', $updateReturnArray[14]['b'] );
   }
   
   function test_count()
   {
      echo "\n---Begin to count after findAndUpdate.\n";
      
      $count = self::$dbh -> countRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare results for findAndUpdate
      $this -> assertEquals( 15, $count );
      
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
      echo "\n---Test 7664_02 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>