/****************************************************
@description:      insert / update / upsert / remove, only cover required parameter
@testlink cases:   seqDB-7657
@input:        1 createCL
               2 insert, $record: array
                         $record: string
               3 find, without condition
               4 update, $rule: array
               5 upsert, $rule: array
               6 remove, without condition
               7 dropCL
@output:     success
@modify list:
        2016-4-20 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DataOperator03 extends BaseOperator 
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
      $recs1 = array( 'a' => 1 );
      $recs2 = json_encode( array( 'a' => 2 ) );
      
      $recsArray = array( $recs1, $recs2 );
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
   }
   
   function findRecs( $clDB )
   {
      $findRecsArray = array() ;
      $cursor = $clDB -> find();
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
      return $findRecsArray;
   }
   
   function updateRecs( $clDB )
   {
      $rule = array( '$inc' => array( 'a' => 2 ) );
      $clDB -> update( $rule );
   }
   
   
   function upsertRecs( $clDB )
   {
      $rule = array( '$set' => array( 'a' => 'test' ) );
      $clDB -> update( $rule );
   }
   
   function removeRecs( $clDB )
   {
      $clDB -> remove();
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestData03 extends PHPUnit_Framework_TestCase
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
   
      self::$dbh = new DataOperator03();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME . '_7657';
      
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
   
   function test_findAfterInsert()
   {
      echo "\n---Begin to find records after insert.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 2, $recsArray );
      $expA1 = 1;  //{a:1}
      $expA2 = 2;  //{a:2}
      for( $i = 0; $i < count($recsArray); $i++ )
      {
         if( $recsArray[$i]['a'] == $expA1 )
         {
            $this -> assertEquals( $expA1, $recsArray[$i]['a'] );
         }
         else if( $recsArray[$i]['a'] == $expA2 )
         {
            $this -> assertEquals( $expA2, $recsArray[$i]['a'] );
         }
      }
   }
   
   function test_update()
   {
      echo "\n---Begin to update records.\n";
      
      self::$dbh -> updateRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_findAfterUpdate()
   {
      echo "\n---Begin to find records after update.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 2, $recsArray );
      $expA1 = 3;  //{a:1} ---update({$inc:{a:2}})
      $expA2 = 4;  //{a:2}
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         if( $recsArray[$i]['a'] == $expA1 )
         {
            $this -> assertEquals( $expA1, $recsArray[$i]['a'] );
         }
         else if( $recsArray[$i]['a'] == $expA2 )
         {
            $this -> assertEquals( $expA2, $recsArray[$i]['a'] );
         }
      }
   }
   
   function test_upsert()
   {
      echo "\n---Begin to upsert records.\n";
      
      self::$dbh -> upsertRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_findAfterUpsert()
   {
      echo "\n---Begin to find records after upsert.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 2, $recsArray );
      $expA = "test";  //{a:1} ---upsert({$set:{a:'test'}})
      $this -> assertEquals( $expA, $recsArray[0]['a'] );
      $this -> assertEquals( $expA, $recsArray[1]['a'] );
   }
   
   function test_remove()
   {
      echo "\n---Begin to remove records.\n";
      
      self::$dbh -> removeRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_findAfterRemove()
   {
      echo "\n---Begin to find records after remove.\n";
      
      $cursor = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 0, $cursor );
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
      echo "\n---Test 7657 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
}
?>