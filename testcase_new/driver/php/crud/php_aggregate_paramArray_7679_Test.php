/****************************************************
@description:      execSQL / execUpdateSQL, base case
@testlink cases:   seqDB-7679
@input:        1 create cs
               2 create cl
               3 insert
               4 aggregate, $aggrObj: array
               5 drop cl
@output:     success
@modify list:
        2016-4-27 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class AggregateOper01 extends BaseOperator 
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
      $clDB -> insert( '{no:2,score:80,name:"Tom", age:12}' );
      $clDB -> insert( '{no:1,score:80,name:"Json",age:13}' );
      $clDB -> insert( '{no:4,score:80,name:"Tom", age:12}' );
      $clDB -> insert( '{no:6,score:92,name:"Tom", age:12}' );
      $clDB -> insert( '{no:3,score:80,name:"Aber",age:15}' );
      $clDB -> insert( '{no:5,score:80,name:"Tom", age:13}' );
      
   }
   
   function aggregateOper( $clDB, $csName, $clName )
   {
      $aggrObj = array( array( '$sort'    => array( 'no' => 1 ) ), 
                        array( '$project' => array( 'no' => 1, 'score' => 1, 'name' => 1 ) ), 
                        array( '$match'   => array( 'score' => 80 ) ), 
                        array( '$limit'   => 4 ), 
                        array( '$skip'    => 1 ), 
                        array( '$group'   => array( '_id' => '$name', 'count' => array( '$count' => '$name' ), 
                            'name' => array( '$first' =>'$name' ) ) ) );
                        
      $cursor = $clDB -> aggregate( $aggrObj );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to find records. Errno: ". $errno ."\n";
      }
      
      $recsArray = array();
      while( $records = $cursor -> next() )
      {
         array_push( $recsArray, $records );
      }
      
      return $recsArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class testAggregate01 extends PHPUnit_Framework_TestCase
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
   
      self::$dbh = new AggregateOper01();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME . '_7679Array';
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB );  //raw data
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_aggregate()
   {
      echo "\n---Begin to exec aggregate.\n";
      
      $recsArray = self::$dbh -> aggregateOper( self::$clDB, self::$csName, self::$clName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 2, $recsArray );
      $this -> assertEquals( 'Aber', $recsArray[0]['name'] );
      $this -> assertEquals( 'Tom',  $recsArray[1]['name'] );
      if( is_object( $recsArray[0]['count'] ) )
      {
         $this -> assertEquals( '1', $recsArray[0]['count'] -> __toString() );
         $this -> assertEquals( '2', $recsArray[1]['count'] -> __toString() );
      }
      else
      {
         $this -> assertEquals( '1', $recsArray[0]['count'] );
         $this -> assertEquals( '2', $recsArray[1]['count'] );
      }
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
      echo "\n---Test 7679 Array spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>