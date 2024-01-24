/****************************************************
@description:      insert, data type is numberLong
@testlink cases:   seqDB-7830
@input:        1 createCL
               2 insert, data type is numberLong, and the data is exceed than maxValue
               3 find and check results
               4 dropCL
@output:     maxValue when the value is greater than the maxValue
             minValue when the value is less than the minValue
@modify list:
        2016-5-4 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class NumerLongType03 extends BaseOperator 
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
      // minValue/maxValue
      $recs = '{"a":{"$numberLong":"-9223372036854775808"}, "b":{"$numberLong":"9223372036854775807"}}';
      $clDB -> insert( $recs );
   }
   
   function findRecs( $clDB )
   {
      //find using '$type'
      $condition = '{ $and:[{a:{$type:1,$et:18}}, {b:{$type:1,$et:18}}] }';
      
      $tmpArray = array() ;
      $cursor = $clDB -> find( $condition );
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

class TestNumberLong03 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new NumerLongType03();
      
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
      
      //maxValue when the value is greater than the maxValue
      //minValue when the value is less than the minValue
      if( is_object( $recsArray[0]['a'] ) )
      {
         $this -> assertEquals( '-9223372036854775808', $recsArray[0]['a'] -> __toString() );
         $this -> assertEquals( '9223372036854775807', $recsArray[0]['b'] -> __toString() );
      }
      else
      {
         $this -> assertEquals( '-9223372036854775808', $recsArray[0]['a'] );
         $this -> assertEquals( '9223372036854775807', $recsArray[0]['b'] );
      }
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