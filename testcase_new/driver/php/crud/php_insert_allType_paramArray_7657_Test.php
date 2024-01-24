/****************************************************
@description:      insert / find, cover all data type
@testlink cases:   seqDB-7657/seqDB-7663
@input:        1 test_createCL
               2 insert, $records: array, cover all data type:
                  $null, $int32, $double, $string, $bool, 
                  $oid, $int64, $date, $binary, $regex, $timestamp, $minKey, $maxKey, 
                  $subobj, $array
               3 find, with condition
                  $condition/$selector/$orderby: array
                  $hint: null ----test $hint when test index
                  $numToSkip/$numToReturn/$flag: int
               4 dropCL
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DataOperator08 extends BaseOperator 
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
      //insert count: 18
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
      //others for find by condition, count: 3
      $tmp1 = array( 'a' => -1, 'c' => 'dataType' );
      $tmp2 = array( 'a' => 15, 'c' => 'dataType' );
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
   
   function findRecs( $clDB )
   {
      $condition = array( 'c' => 'dataType' );
      $selector  = array( 'a' => '', 'b' => '', 'c' => '', 'd' => 'hello' );
      $orderby   = array( 'a' => 1 );
      $hint      = null;  //test $hint when test index
      $numToSkip = 1;
      $numToReturn = 15;
      $flag = 0;
      
      $cursor = $clDB -> find( $condition, $selector, $orderby, $hint, 
                               $numToSkip, $numToReturn, $flag );
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
      return $findRecsArray;
   }
   
   function findMatchType( $clDB, $findRecsArray )
   {
      //   base type: 10---null; 16---int32; 1---double; 2---string; 8---boll; 4---array;
      //special type: 7---oid; 18---int64; 9---date; 5---binary; 11---regex; 17---timestamp;
      $matchType = array( 10, 16, 1, 2, 8, 4,   7, 18, 9, 5, 11, 17 );
      for( $i = 0; $i < count( $matchType ); $i++ ) //count( $typeArray )
      {
         //exec find by $type
         $condition = '{"b": {"$type":1, "$et":'. $matchType[$i] .'}}';
         $cursor = $clDB -> find( $condition );
         
         $errno = $this -> getErrno();
         if( $errno !== 0 )
         {
            echo "\nFailed to exec find. Errno: ". $errno ."\n";
         }
         
         //compare data type by $type
         while( $tmpInfo = $cursor -> next() )
         {
            //for base type
            if( $i < 6 )
            {
               if( $tmpInfo['b'] !== $findRecsArray[$i]['b'] )
               {
                  echo "\nFailed to compared records of base type.\n";
                  echo "Match type: ". $matchType[$i] ."\n";
                  echo "expect value of key[b]: \n";
                  var_dump( $tmpInfo[$i]['b'] -> __toString() ) ."\n";
                  echo "actual value of key[b]: \n";
                  var_dump( $findRecsArray[$i]['b'] -> __toString() ) ."\n";
                  
                  return false;
               }
            }
            //for special type
            else if( 6 <= $i && $i < count( $matchType ) )
            {
               if( is_object($findRecsArray[$i]['b']) )
               {
                  $expResult = $tmpInfo['b'] -> __toString();
                  $actResult = $findRecsArray[$i]['b'] -> __toString();
               }
               else
               {
                  $expResult = $tmpInfo['b'];
                  $actResult = $findRecsArray[$i]['b'] ;
               }
               if( $expResult !== $actResult )
               {  
                  echo "\nFailed to compared records of special type.\n";
                  echo "Match type: ". $matchType[$i] ."\n";
                  echo "expect value of key[b]: \n";
                  var_dump( $expResult ) ."\n";
                  echo "actual value of key[b]: \n";
                  var_dump( $actResult ) ."\n";
                  
                  return false;
               }
            }
         }
      }
      //compare type: 3---subobj  //bug: returns contain array
      $condition = '{"b": {"$type":1, "$et":3 }}';
      $orderby   = array( 'a' => -1 );
      $cursor = $clDB -> find( $condition, null, $orderby );
      $errno = $this -> getErrno();
      if( $errno !== 0 ){ echo "\nFailed to exec find. Errno: ". $errno ."\n"; }
      $tmpInfo = $cursor -> current();
      if( $tmpInfo['b'] !== $findRecsArray[14]['b'] )
      {
         echo "\nFailed to compared records of base type.\n";
         echo "Match type: 3--subobj \n";
         echo "expect value of key[b]: \n";
         var_dump( $tmpInfo['b'] ) ."\n";
         echo "actual value of key[b]: \n";
         var_dump( $findRecsArray[14]['b'] ) ."\n";
         
         return false;
      }
      
      return true;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestData08 extends PHPUnit_Framework_TestCase
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
   
      self::$dbh = new DataOperator08();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME . '_7657_01';
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$rawData = self::$dbh -> insertRecs( self::$clDB );
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
      for( $i = 0; $i < count( self::$rawData ) - 3; $i++ ) //3 records is filtered out
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
      
      //------------------------compare data type---------------------------------------------
      echo "\n   Begin to compare data type.\n";
      //compare data type: null, int32, double, string, bool, array, oid, int64, date, binary, regex, timestamp, subobj
      $results = self::$dbh -> findMatchType( self::$clDB, $findRecsArray );
      $this -> assertTrue( $results );
      
      //compare type: minkey
      $actMinkey = $findRecsArray[12]['b'];
      $this -> assertTrue( is_object( $actMinkey ) && is_a( $actMinkey, 'SequoiaMinKey' ) ) ;
      
      //compare type: maxKey
      $actResults = $findRecsArray[13]['b'];
      $this -> assertTrue( is_object( $actResults ) && is_a( $actResults, 'SequoiaMaxKey' ) ) ;
      
      //-----------------------compare find condition---------------------------------------------
      echo "\n   Begin to compare find condition.\n";
      //compare $condition
      for( $i = 0; $i < count( $findRecsArray ); $i++ )
      {
         $expC = 'dataType';
         $actC = $findRecsArray[$i]['c'];
         $this -> assertEquals( $expC, $actC );
      }
      
      //compare $selector
      for( $i = 0; $i < count( $findRecsArray ); $i++ )
      {
         $expD = 'hello';
         $actD = $findRecsArray[$i]['d'];
         $this -> assertEquals( $expD, $actD );
      }
      
      //compare $orderby
      for( $i = 0; $i < count( $findRecsArray ); $i++ )
      {
         $expA = $i;
         $actA = $findRecsArray[$i]['a'];
         $this -> assertEquals( $expA, $actA );
      }
      
      //compare $numToSkip
      $expA = -1;
      $actA = array();
      for( $i = 0; $i < count( $findRecsArray ); $i++ )
      {
         array_push( $actA, $findRecsArray[$i]['a'] );
      }
      $this -> assertNotContains( $expA, $actA );
      
      //compare $numToReturn
      $expA = 15;
      $actA = array();
      for( $i = 0; $i < count( $findRecsArray ); $i++ )
      {
         array_push( $actA, $findRecsArray[$i]['a'] );
      }
      $this -> assertNotContains( $expA, $actA );
      
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
      echo "\n---Test 7657_01 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>