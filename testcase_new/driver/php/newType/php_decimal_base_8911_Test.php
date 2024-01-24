/****************************************************
@description:      insert decimal, by __construct
@testlink cases:   seqDB-8911
@input:        1 insert, data type: decimal
@output:     success
@modify list:
        2016-7-7 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DecimalType03 extends BaseOperator 
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
      $recs0 = array( 'a' => 0, 'b' => new SequoiaDecimal( '1.123' ) );
      $recs1 = array( 'a' => 1, 'b' => new SequoiaDecimal( '1.123', -1 ) );
      $recs2 = array( 'a' => 2, 'b' => new SequoiaDecimal( '1.123', 1000, 999 ) );
      $recs3 = array( 'a' => 3, 'b' => new SequoiaDecimal( '1.123', 1, 0 ) );
      
      $recsArray = array( $recs0, $recs1, $recs2, $recs3 );
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
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

class TestDecimal03 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DecimalType03();
      
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
      echo "\n---Begin to find records by type[100].\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 4, $recsArray );
      $expDecimal0 = "1.123"; 
      $expDecimal1 = "1.123"; 
      $expDecimal2 = "1.123000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"; 
      $expDecimal3 = "1";
      $this -> assertEquals( $expDecimal0, $recsArray[0]['b'] );
      $this -> assertEquals( $expDecimal1, $recsArray[1]['b'] );
      $this -> assertEquals( $expDecimal2, $recsArray[2]['b'] );
      $this -> assertEquals( $expDecimal3, $recsArray[3]['b'] );
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