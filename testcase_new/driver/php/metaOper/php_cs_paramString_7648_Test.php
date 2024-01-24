/****************************************************
@description:      createCS / listCS, cover all param, and type is string
@testlink cases:   seqDB-7645 / seqDB-7650
@input:      1 test_createCS, $options: string
             2 test_listCS, cover all param, $condition/$selector/$orderBy/$hint: null
                     -------[from doc]This parameter is reserved and must be null
             3 test_dropCS
@output:     success
@modify list:
        2016-4-15 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator05 extends BaseOperator 
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
   
   function snapshotOper( $csName )
   {
      $condition = json_encode( array( 'Name' => $csName ) );
      $cursor = $this -> db -> snapshot( SDB_SNAP_COLLECTIONSPACE, $condition );
      
      return $cursor -> current();
   }
   
   function createCS( $csName )
   {
      $options = json_encode( array( 'PageSize' => 8192 ) );
      $this -> commCreateCS( $csName, $options );
   }
   
   //get csInfo by snapshot need cl, else snapshot result is null
   function createCL( $csName, $clName )
   {
      $options = array( 'ReplSize' => 1 );
      return $this -> commCreateCL( $csName, $clName, $options );
   }
   
   function insertRecs( $clDB )
   {
      $clDB -> insert( array( 'a' => 1 ) );
   }
   
   function listCS()
   {
      $condition = null;
      $selector  = null;
      $orderby   = null;
      $hint = null;
      
      $cursor = $this -> db -> listCS( $condition, $selector, $orderby, $hint );
      $tmpArray = array();
      while( $tmpInfo = $cursor -> next() )
      {
         array_push( $tmpArray, $tmpInfo['Name'] );
      }
      
      return $tmpArray;
   } 
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
}

class TestCS05 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CSOperator05();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME ."_str7648";
      self::$clName = self::$dbh -> COMMCLNAME ."_str7648";
      
      echo "\n---Begin to drop cs[". self::$csName ."] in the begin.\n";
      self::$dbh -> dropCS( self::$csName, true );
   }
   
   function test_createCS()
   {
      echo "\n---Begin to create cs[". self::$csName ."].\n";
      
      self::$dbh -> createCS( self::$csName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_createCL()
   {
      echo "\n---Begin to create cl[". self::$csName .".". self::$clName ."].\n";
      
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_listCS()
   {
      echo "\n---Begin to list cs.\n";
      
      $tmpArray = self::$dbh -> listCS();
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare results[Name]
      $this -> assertContains( self::$csName,  $tmpArray );
   }
   
   function test_snapshot()
   {
      echo "\n---Begin to check results by snapshot.\n";
      
      $csInfo = self::$dbh -> snapshotOper( self::$csName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $name = $csInfo['Name'];
      $pageSize = $csInfo['PageSize'];
      $this -> assertEquals( self::$csName, $name );
      $this -> assertEquals( 8192, $pageSize );
   }
   
   function test_dropCS()
   {
      echo "\n---Begin to drop cs[". self::$csName ."] in the end.\n";
      
      self::$dbh -> dropCS( self::$csName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>