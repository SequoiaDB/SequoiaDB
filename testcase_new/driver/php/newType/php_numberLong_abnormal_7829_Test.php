/****************************************************
@description:      insert, abnormal case
@testlink cases:   seqDB-7828
@input:        1 createCL
               2 insert, data type is numberLong, and format error
               3 find and check results
               4 dropCL
@output:     errno: 
@modify list:
        2016-5-4 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class NumerLongType01 extends BaseOperator 
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
   
   function insertRecs( $clDB, $recs )
   {
      $clDB -> insert( $recs );
   }
   
   function findRecs( $clDB )
   {
      $tmpArray = array() ;
      $cursor = $clDB -> find();
      while( $record = $cursor -> next() )
      {
         array_push( $tmpArray, $record );
      }
      return $tmpArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestNumberLong01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new NumerLongType01();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME;
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      $recs = '{"a":{"$numberLong": 12}}}';
      self::$dbh -> insertRecs( self::$clDB, $recs );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -258, $errno );
      
      $recs = '{"a":{"$numberLong": "12test"}}}';
      self::$dbh -> insertRecs( self::$clDB, $recs );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -258, $errno );
   }
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
   }
   
   function test_dropCL()
   {
      echo "\n---Begin to drop cl in the end.\n";
      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>