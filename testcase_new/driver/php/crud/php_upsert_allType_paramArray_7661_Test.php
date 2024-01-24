/****************************************************
@description:      upsert, cover all data type
@testlink cases:   seqDB-7660
@input:        1 test_createCL
               2 insert, cover all data type:
                  $null, $int32, $double, $string, $bool, $subobj, $array, 
                  $oid, $int64, $date, $binary, $regex, $timestamp, $minKey, $maxKey
               3 find, with condition
               4 upsert, with condition cover all data type, $rule/$condition: array
                 $hint:null  ---test $hint when test index
               5 count, $condition: array
                 $hint:null  ---test $hint when test index
               6 dropCL
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DataOperator16 extends BaseOperator 
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
      $oid    = array( 'a' => 6,  'b' => new SequoiaID( '123abcd00ef12358902300ef' ), 'c' => 'dataType' );
      $int64  = array( 'a' => 7,  'b' => new SequoiaINT64( "-9223372036854775808" ), 'c' => 'dataType' );
      $date   = array( 'a' => 8,  'b' => new SequoiaDate( '1991-11-27' ), 'c' => 'dataType' );
      $binary = array( 'a' => 9,  'b' => new SequoiaBinary( 'aGVsbG8gd29ybGQ=', '1' ), 'c' => 'dataType' );
      $regex  = array( 'a' => 10, 'b' => new SequoiaRegex( '^rg', 'i' ), 'c' => 'dataType' );
      $timestamp = array( 'a' => 11, 'b' => new SequoiaTimestamp( '1901-12-15-00.00.00.000000' ), 'c' => 'dataType' );
      $minKey = array( 'a' => 12, 'b' => new SequoiaMinKey(), 'c' => 'dataType' );
      $maxKey = array( 'a' => 13, 'b' => new SequoiaMaxKey(), 'c' => 'dataType' );
      $subobj = array( 'a' => 14, 'b' => array( 'subobj' => "111" ), 'c' => 'dataType' );
      
      $recsArray = array( $null, $int32, $double, $string, $bool, $array, 
                          $oid, $int64, $date, $binary, $regex, $timestamp, $minKey, $maxKey, 
                          $subobj );
      
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
      
      return $recsArray;
   }
   
   function findRecs( $clDB )
   {
      $orderby   = array( 'a' => 1 );
      $cursor = $clDB -> find( null, null, $orderby );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to find records. Errno: ". $errno ."\n";
      }
      
      $findRecsArray = array() ;
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
      //var_dump( $findRecsArray );
      return $findRecsArray;
   }
   
   function upsertRecs( $clDB, $recsArray )
   {
      $rule = array( '$set' => array( 'b' => 'hello' ) );
      $hint = null;  //test $hint when test index
      for( $i = 0; $i < count( $recsArray ); $i++ ) 
      {
         if( $i < 12 || $i > 13 ) //last record is minKey/maxKey, can not match the records to remove 
         {
            $condition = array( 'b' => $recsArray[$i]['b'] );
         }
         else if( $i === 12 || $i === 13 ) //minKey maxKey
         {
            $condition = array( 'a' => $recsArray[$i]['a'] );
         }
         
         $clDB -> upsert( $rule, $condition, $hint );
      }
   }
   
   function countRecs( $clDB )
   {
      $condition = array( 'b' => 'hello' );
      $hint      = null;  //test $hint when test index
      return $clDB -> count( $condition, $hint );
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestData16 extends PHPUnit_Framework_TestCase
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
   
      self::$dbh = new DataOperator16();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME . '_7661_01';
      
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
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $findRecsArray = self::$dbh -> findRecs( self::$clDB );
      //compare exec result
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //-------------------------compare records------------------------------------
      echo "\n   Begin to compare records.\n";
      //compare count
      $expCount = 15;
      $this -> assertCount( $expCount, $findRecsArray );
      
      //get field b
      $expRecsArray = array();
      for( $i = 0; $i < count( self::$rawData ); $i++ )
      {
         array_push( $expRecsArray, self::$rawData[$i]['b'] );
      }
      
      //compare result
      for( $i = 0; $i < count($expRecsArray); $i++ )
      {
         $expResults = $expRecsArray[$i];
         $actResults = $findRecsArray[$i]['b'];
         if( $i < 6 )
         {
            $this -> assertEquals( $expResults, $actResults );
         }
         else if( 6 <= $i && $i < count($expRecsArray) - 3 )
         {
            if( is_object( $actResults ) )
            {
               $actResults = $actResults -> __toString();
            }
            else
            {
               $expResults = $expResults -> __toString();
            }
            $this -> assertEquals( $expResults, $actResults );
         }
      }
      
      //compare minkey
      $actMinkey = $findRecsArray[12]['b'];
      $this -> assertTrue( is_object( $actMinkey ) && is_a( $actMinkey, 'SequoiaMinKey' ) ) ;
      
      //compare maxKey
      $actMaxkey = $findRecsArray[13]['b'];
      $this -> assertTrue( is_object( $actMaxkey ) && is_a( $actMaxkey, 'SequoiaMaxKey' ) ) ;
      
      //compare subobj
      $this -> assertEquals( $expRecsArray[14], $findRecsArray[14]['b'] );
   }
   
   function test_upsert()
   {
      echo "\n   Begin to upsert records.\n";
      
      self::$dbh -> upsertRecs( self::$clDB, self::$rawData );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_count()
   {
      echo "\n   Begin to count records for upsert.\n";
      
      $actCount = self::$dbh -> countRecs( self::$clDB );
      //compare exec result
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare count
      $expCount = 15;
      $this -> assertEquals( $expCount, $actCount );
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
      echo "\n---Test 7661_01 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>