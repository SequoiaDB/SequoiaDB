/****************************************************
@description:      query( findAndUpdate, findAndRemove ), base case
@testlink cases:   seqDB-7664
@input:        1 createCL
               2 insert records
               3 findAndUpdate, only cover required paramater
               4 findAndRemove, only cover required paramater
@output:     success
@modify list:
        2016-6-20 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DataOperator10 extends BaseOperator 
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
      $recs1 = array( 'a' => 0 );
      $recs2 = json_encode( array( 'a' => 1 ) );
      
      $recsArray = array( $recs1, $recs2 );
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
   }
   
   function findAndUpdateRecs( $clDB )
   {
      $rule = array( '$inc' => array( 'a' => 1 ) );
      $cursor = $clDB -> findAndUpdate( $rule );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to findAndUpdate. Errno: ". $errno ."\n";
      }
      
      while( $tmpInfo = $cursor -> next() )
      {
         //continue;
      }
   }
   
   function findAndRemoveRecs( $clDB )
   {
      $findRecsArray = array() ;
      $cursor = $clDB -> findAndRemove();
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to findAndRemove. Errno: ". $errno ."\n";
      }
      
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
   }
   
   function findRecs( $clDB )
   {
      $findRecsArray = array() ;
      $cursor = $clDB -> find();
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to find. Errno: ". $errno ."\n";
      }
      
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

class TestData10 extends PHPUnit_Framework_TestCase
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
   
      self::$dbh = new DataOperator10();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName  = self::$dbh -> COMMCLNAME . '_7664_03';
      
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
   
   function test_findAndUpdate()
   {
      echo "\n---Begin to findAndUpdata.\n";
      
      $updateReturnArray = self::$dbh -> findAndUpdateRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
   }
   
   function test_findAfterQuery1()
   {
      echo "\n---Begin to find records after findAndUpdate.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 2, $recsArray );
      $expA1 = 1;  //{a:1}
      $expA2 = 2;  //{a:2}
      if( $recsArray[0]['a'] == $expA1 )
      {
         $this -> assertEquals( $expA2, $recsArray[1]['a'] );
      }
      else if( $recsArray[0]['a'] == $expA2 )
      {
         $this -> assertEquals( $expA1, $recsArray[1]['a'] );
      }
   }
   
   function test_findAndRemove()
   {
      echo "\n---Begin to findAndRemove.\n";
      
      $updateReturnArray = self::$dbh -> findAndRemoveRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
   }
   
   function test_findAfterQuery2()
   {
      echo "\n---Begin to find records after findAndRemove.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 0, $recsArray );
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
      echo "\n---Test 7664_03 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>