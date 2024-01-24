/****************************************************
@description:      insert decimal, normal case
@testlink cases:   seqDB-8913
@input:        1 insert, data type: decimal
@output:     success
@modify list:
        2016-7-7 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DecimalType01 extends BaseOperator 
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
      $findRecsArray = array() ;
      $cursor = $clDB -> find( '{b:{$type:1,$et:100}}', null, '{a:1}' );   //100: decimal type
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
      //var_dump( $findRecsArray );
      return $findRecsArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestDecimal01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DecimalType01();
      
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
      
      //$decimal is invalid
      $recs = '{"a":{"$decimal": 12}}}';
      self::$dbh -> insertRecs( self::$clDB, $recs );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -258, $errno );
      
      //$precision is invalid
      $recs = '{"a":{"$decimal": "1.123", $precision:11}}}';
      self::$dbh -> insertRecs( self::$clDB, $recs );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -258, $errno );
      
      //$precision out of the minValue
      $recs = '{"a":{"$decimal": "1.123", $precision:[0,-1]}}}';
      self::$dbh -> insertRecs( self::$clDB, $recs );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -258, $errno );
      
      //$precision out of the maxValue
      $recs = '{"a":{"$decimal": "1.123", $precision:[1001,1000]}}}';
      self::$dbh -> insertRecs( self::$clDB, $recs );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -258, $errno );
      
      //$scale greater than $precision
      $recs = '{"a":{"$decimal": "1.123", $precision:[11,12]}}}';
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