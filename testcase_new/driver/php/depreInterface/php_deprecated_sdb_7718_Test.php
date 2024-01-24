/****************************************************
@description:      deprecated interface, base case
@testlink cases:   seqDB-7716/ seqDB-7717/ seqDB-7718
@input:        1 getSnapshot
               2 getList  ---bug: jira-1692  see: function test_getList()
               3 getNext
@output:     success
@modify list:
        2016-4-29 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DepreOperator01 extends BaseOperator 
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
   
   function selectCS( $csName )
   { 
      return $this -> db -> selectCS( $csName );
   }
   
   function selectCollection( $csDB, $clName )
   {
      return $csDB -> selectCollection( $clName, null );
   }
   
   function getSnapshot( $csName, $clName2, $clName3 )
   {
      $type = SDB_SNAP_COLLECTION;
      $condition = array( 'Name' => array( '$in' => array( $csName .'.'. $clName2, $csName .'.'. $clName3 ) ) );
      $selector  = array( 'Name' => '', 'd' => 'hello' );
      $orderby   = array( 'Name' => -1 );
      
      $cursor = $this -> db -> getSnapshot( $type, $condition, $selector, $orderby );
      
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to exec snapshot. Errno: ". $errno ."\n";
      }
      
      $tmpArray = array();
      while( $tmpInfo = $cursor -> getNext() )
      {
         array_push( $tmpArray, $tmpInfo );
      }
      
      return $tmpArray;
   }
   
   function getList( $csName, $clName2, $clName3 )
   {
      $type = SDB_SNAP_COLLECTION;
      $condition = array( 'Name' => array( '$in' => array( $csName .'.'. $clName2, $csName .'.'. $clName3 ) ) );
      $selector  = array( 'Name' => '', 'd' => 'hello' );
      $orderby   = array( 'Name' => -1 );
      
      $cursor = $this -> db -> getList( $type, $condition, $selector, $orderby );
      
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to exec list. Errno: ". $errno ."\n";
      }
      
      $tmpArray = array();
      while( $tmpInfo = $cursor -> getNext() )
      {
         array_push( $tmpArray, $tmpInfo );
      }
      
      return $tmpArray;
   }
   
   function dropCollectionSpace( $csName )
   {
      $this -> db -> dropCollectionSpace( $csName );
   }
   
}

class TestDepre01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName1;
   private static $clName2;
   private static $clName3;
   private static $csDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DepreOperator01();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName1 = self::$dbh -> COMMCLNAME .'_1';
      self::$clName2 = self::$dbh -> COMMCLNAME .'_2';
      self::$clName3 = self::$dbh -> COMMCLNAME .'_3';
      
      echo "\n---Begin to drop cs in the begin.\n";
      self::$dbh -> dropCollectionSpace( self::$csName );
      
      echo "\n---Begin to create cs in the begin.\n";
      self::$csDB = self::$dbh -> selectCS( self::$csName );
      
      echo "\n---Begin to create cl in the begin.\n";
      self::$dbh -> selectCollection( self::$csDB, self::$clName1 );
      self::$dbh -> selectCollection( self::$csDB, self::$clName2 );
      self::$dbh -> selectCollection( self::$csDB, self::$clName3 );
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_getSnapshot()
   {
      echo "\n---Begin to getSnapshot.\n";
      $clArray = self::$dbh -> getSnapshot( self::$csName, self::$clName2, self::$clName3 );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare for $condition
      $this -> assertCount( 2, $clArray );
      //compare for $orderby
      $this -> assertEquals( self::$csName .'.'. self::$clName3, $clArray[0]['Name'] );
      $this -> assertEquals( self::$csName .'.'. self::$clName2, $clArray[1]['Name'] );
      //compare for $selector
      $this -> assertEquals( 'hello', $clArray[1]['d'] );
   }
   
   function test_getList()
   {
      echo "\n---Begin to getList.\n";
      
      $clArray = self::$dbh -> getList( self::$csName, self::$clName2, self::$clName3 );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare for $condition
      $this -> assertCount( 2, $clArray );
      //compare for $orderby
      $this -> assertEquals( self::$csName .'.'. self::$clName3, $clArray[0]['Name'] );
      $this -> assertEquals( self::$csName .'.'. self::$clName2, $clArray[1]['Name'] );
      //compare for $selector
      $this -> assertEquals( 'hello', $clArray[1]['d'] );
   }
   
   function test_dropCollectionSpace()
   {
      echo "\n---Begin to dropCollectionSpace.\n";
      
      self::$dbh -> dropCollectionSpace( self::$csName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
}
?>