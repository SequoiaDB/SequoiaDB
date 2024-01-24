/****************************************************
@description:      insert decimal, by __construct
@testlink cases:   seqDB-14015
@input:        1 insert, data type: decimal, special value: MAX/MIN/NAN/-INF/INF
@output:     success
@modify list:
        2018-3-13 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DecimalType14015 extends BaseOperator 
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
      $recs0 = array( 'a' => 0, 'b' => new SequoiaDecimal( 'MAX' ) );
      $recs1 = array( 'a' => 1, 'b' => new SequoiaDecimal( 'MIN' ) );
      $recs2 = array( 'a' => 2, 'b' => new SequoiaDecimal( 'NAN' ) );
      $recs3 = array( 'a' => 3, 'b' => new SequoiaDecimal( 'INF' ) );
      $recs4 = array( 'a' => 4, 'b' => new SequoiaDecimal( '-INF' ) );
      $recs5 = array( 'a' => 5, 'b' => new SequoiaDecimal( 'max' ) );
      $recs6 = array( 'a' => 6, 'b' => new SequoiaDecimal( 'min' ) );
      $recs7 = array( 'a' => 7, 'b' => new SequoiaDecimal( 'nan' ) );
      $recs8 = array( 'a' => 8, 'b' => new SequoiaDecimal( 'inf' ) );
      $recs9 = array( 'a' => 9, 'b' => new SequoiaDecimal( '-inf' ) );
      
      $recsArray = array( $recs0, $recs1, $recs2, $recs3, $recs4, $recs5, $recs6, $recs7, $recs8, $recs9 );
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {  
         //var_dump("i = " + $i);
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
   
   function removeRecs( $clDB )
   {
      $recs0 = array( 'a' => 0, 'b' => new SequoiaDecimal( 'MAX' ) );
      $recs1 = array( 'a' => 1, 'b' => new SequoiaDecimal( 'MIN' ) );
      $recs2 = array( 'a' => 2, 'b' => new SequoiaDecimal( 'NAN' ) );
      $recs3 = array( 'a' => 3, 'b' => new SequoiaDecimal( 'INF' ) );
      $recs4 = array( 'a' => 4, 'b' => new SequoiaDecimal( '-INF' ) );
      $recs5 = array( 'a' => 5, 'b' => new SequoiaDecimal( 'max' ) );
      $recs6 = array( 'a' => 6, 'b' => new SequoiaDecimal( 'min' ) );
      $recs7 = array( 'a' => 7, 'b' => new SequoiaDecimal( 'nan' ) );
      $recs8 = array( 'a' => 8, 'b' => new SequoiaDecimal( 'inf' ) );
      $recs9 = array( 'a' => 9, 'b' => new SequoiaDecimal( '-inf' ) );
      
      $recsArray = array( $recs0, $recs1, $recs2, $recs3, $recs4, $recs5, $recs6, $recs7, $recs8, $recs9 );
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {  
         //var_dump("i = " + $i);
         $clDB -> remove( $recsArray[$i] );
      }
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestDecimal14015 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DecimalType14015();
      
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
      
      $this -> assertCount( 10, $recsArray );
      $expDecimal0 = "MAX"; 
      $expDecimal1 = "MIN"; 
      $expDecimal2 = "NaN"; 
      $this -> assertEquals( $expDecimal0, $recsArray[0]['b'] );
      $this -> assertEquals( $expDecimal0, $recsArray[3]['b'] );
      $this -> assertEquals( $expDecimal0, $recsArray[5]['b'] );
      $this -> assertEquals( $expDecimal0, $recsArray[8]['b'] );
      
      $this -> assertEquals( $expDecimal1, $recsArray[1]['b'] );
      $this -> assertEquals( $expDecimal1, $recsArray[4]['b'] );
      $this -> assertEquals( $expDecimal1, $recsArray[6]['b'] );
      $this -> assertEquals( $expDecimal1, $recsArray[9]['b'] );
      
      $this -> assertEquals( $expDecimal2, $recsArray[2]['b'] );
      $this -> assertEquals( $expDecimal2, $recsArray[7]['b'] );
   }
   
   function test_remove()
   {
      echo "\n---Begin to remove records.\n";
      
      self::$dbh -> removeRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );      
      $this -> assertCount( 0, $recsArray );      
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to drop cl in the end.\n";
      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      $errno = self::$dbh -> getErrno();
      if ($errno != 0) {
          throw new Exception("failed to drop cl, errno=".$errno);
      }
   }
   
}
?>