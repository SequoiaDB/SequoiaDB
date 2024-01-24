/****************************************************
@description:      snapshot, base case
@testlink cases:   seqDB-7629
@input:      1 createCS
             2 snapshot, 
               1) cover only required param, $type: integer 
               2) cover all param, $condition/$selector/$orderBy: array/string
                     $hint: null  -------[from doc]This parameter is reserved and must be null
             3 dropCS
@output:     success
@modify list:
        2016-4-15 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SdbSnapshot extends BaseOperator 
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
   
   function snapshotOper( $csName, $clName2, $clName3, $paramNum )
   {
      $type = SDB_SNAP_COLLECTION;
      
      if( $paramNum === 'mustParam' )
      {  
         $cursor = $this -> db -> snapshot( $type );
      }
      else if( $paramNum === 'allParamArray' )
      {
         $condition = array( 'Name' => array( '$in' => array( $csName .'.'. $clName2, $csName .'.'. $clName3 ) ) );
         $selector  = array( 'Name' => '', 'Details' => '', 'd' => 'hello' );
         $orderby   = array( 'Name' => -1 );
         $hint      = null;  //[from doc]This parameter is reserved and must be null.
         
         $cursor = $this -> db -> snapshot( $type, $condition, $selector, $orderby, $hint );
      }
      else if( $paramNum === 'allParamString' )
      {
         $condition = '{"Name": {"$in": ["'. $csName .'.'. $clName2 .'", "'. $csName .'.'. $clName3 .'"]}}';
         $selector  = json_encode( array( 'Name' => '', 'Details' => '', 'd' => 'hello' ) );
         $orderby   = json_encode( array( 'Name' => -1 ) );
         $hint      = null;  //[from doc]This parameter is reserved and must be null.
         
         $cursor = $this -> db -> snapshot( $type, $condition, $selector, $orderby, $hint );
      }
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to exec snapshot. Errno: ". $errno ."\n";
      }
      
      $tmpArray = array();
      while( $tmpInfo = $cursor -> next() )
      {
         array_push( $tmpArray, $tmpInfo );
      }
      
      return $tmpArray;
   }
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
}

class TestSdbSnapshot extends PHPUnit_Framework_TestCase
{
   
   protected static $dbh;
   private static $csName;
   private static $clName1;
   private static $clName2;
   private static $clName3;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SdbSnapshot();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME ."_snapshot";
      self::$clName1 = self::$dbh -> COMMCLNAME ."_1";
      self::$clName2 = self::$dbh -> COMMCLNAME ."_2";
      self::$clName3 = self::$dbh -> COMMCLNAME ."_3";
      
      echo "\n---Begin to drop cs in the begin.\n";
      self::$dbh -> dropCS( self::$csName, true );
      
      echo "\n---Begin to create cs/cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName1 );
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName2 );
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName3 );
   }
   
   function test_snapshotByParamMust()
   {
      echo "\n---Begin to exec snapshot[by required parameter].\n";
      
      $clArray = self::$dbh -> snapshotOper( self::$csName, self::$clName2, self::$clName3, 'mustParam' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare for $condition
      $this -> assertGreaterThanOrEqual( 3, count( $clArray ) );
   }
   
   function test_snapshotByParamArray()
   {
      echo "\n---Begin to exec snapshot[by all parameter, type: Array].\n";
      
      $clArray = self::$dbh -> snapshotOper( self::$csName, self::$clName2, self::$clName3, 'allParamArray' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare for $condition
      $this -> assertCount( 2, $clArray );
      //compare for $orderby
      $this -> assertEquals( self::$csName .'.'. self::$clName3, $clArray[0]['Name'] );
      $this -> assertEquals( self::$csName .'.'. self::$clName2, $clArray[1]['Name'] );
      //compare for $selector
      $this -> assertEquals( 'hello', $clArray[1]['d'] );
      $this -> assertNotEmpty( $clArray[1]['Details'] );
   }
   
   function test_snapshotByParamString()
   {
      echo "\n---Begin to exec snapshot[by all parameter, type: String].\n";
      
      $clArray = self::$dbh -> snapshotOper( self::$csName, self::$clName2, self::$clName3, 'allParamString' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare for $condition
      $this -> assertCount( 2, $clArray );
      //compare for $orderby
      $this -> assertEquals( self::$csName .'.'. self::$clName3, $clArray[0]['Name'] );
      $this -> assertEquals( self::$csName .'.'. self::$clName2, $clArray[1]['Name'] );
      //compare for $selector
      $this -> assertEquals( 'hello', $clArray[1]['d'] );
      $this -> assertNotEmpty( $clArray[1]['Details'] );
   }
   
   function test_dropCS()
   {
      echo "\n---Begin to drop cs in the end.\n";
      
      self::$dbh -> dropCS( self::$csName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>