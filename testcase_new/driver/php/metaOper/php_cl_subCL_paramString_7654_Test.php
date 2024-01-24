/****************************************************
@description:      attachCL / detachCL , base case
@testlink cases:   seqDB-7654
@input:        1 test_createCS/test_createCL
               2 test_getCS/test_getCL
               3 test_attachCL, $options: string
               4 test_snapshotForAttach
               5 test_detachCL
               6 test_snapshotForDetach
               7 test_dropCL/test_dropCS
@output:     success
@modify list:
        2016-4-20 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CLOperator05 extends BaseOperator 
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
   
   function snapshotOper( $csName, $clName )
   {
      $cl = $csName .'.'. $clName;
      $condition = array( 'Name' => $cl );
      $cursor = $this -> db -> snapshot( SDB_SNAP_CATALOG, $condition );
      $clInfo = $cursor -> current();
      
      return $clInfo;
   }
   
   function createCS( $csName )
   {
      return $this -> commCreateCS( $csName );
   }
   
   function createCL( $csName, $clName, $type )
   {
      if( $type === 'mainCL' )
      {
         $options = array( 'ShardingKey' => array( 'a' => 1 ), 'IsMainCL' => true );
      }
      else if( $type === 'subCL' )
      {
         $options = array( 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'hash' );
      }
      $cl = $this -> commCreateCL( $csName, $clName, $options, true );
      
      return $cl;
   }
   
   function getCS( $csName )
   {
      $csDB = $this -> db -> getCS( $csName );
      return $csDB;
   } 
   
   function getCL( $csDB, $clName )
   {
      $clDB = $csDB -> getCL( $clName ); 
      return $clDB;
   } 
   
   function attachCL( $mainCL, $subFullName )
   {
      $options = json_encode( array( 'LowBound' => array( 'a' => 0 ), 'UpBound' => array( 'a' => 100 ) ) );
      
      $mainCL -> attachCL( $subFullName, $options );
   }
   
   function detachCL( $mainCL, $subFullName )
   {
      $mainCL -> detachCL( $subFullName );
   }
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestCL05 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $mainCLName;
   private static $subCLName;
   private static $csDB;
   private static $mainCL;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CLOperator05();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$mainCLName = self::$dbh -> COMMCLNAME .'_mainCL';
      self::$subCLName  = self::$dbh -> COMMCLNAME .'_subCL';
      
      echo "\n---Begin to drop cs/cl in the begin.\n";
      self::$dbh -> dropCS( self::$csName, true );
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_createCS()
   {
      echo "\n---Begin to create cs.\n";
      
      self::$dbh -> createCS( self::$csName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_createCL()
   {
      echo "\n---Begin to create cl.\n";
      
      //mainCL
      self::$dbh -> createCL( self::$csName, self::$mainCLName, 'mainCL' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //subCL
      self::$dbh -> createCL( self::$csName, self::$subCLName, 'subCL' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   
   function test_getCS()
   {
      echo "\n---Begin to get cs.\n";
      
      self::$csDB = self::$dbh -> getCS( self::$csName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_getCL()
   {
      echo "\n---Begin to get cl.\n";
      
      self::$mainCL = self::$dbh -> getCL( self::$csDB, self::$mainCLName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_attachCL()
   {
      echo "\n---Begin to attach cl.\n";
      
      $subFullName = self::$csName .'.'. self::$subCLName;
      self::$dbh -> attachCL( self::$mainCL, $subFullName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_snapshotForAttach()
   {
      echo "\n---Begin to check results by snapshot.\n";
      //when create cs by opthon, need to using snapshot and check attribute of options
      
      //----------check results of mainCL----------------
      $mainInfo = self::$dbh -> snapshotOper( self::$csName, self::$mainCLName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //expect value
      $expKey  = array( 'a' => 1 );
      $expType = 'range';
      $expSubCL    = self::$csName .'.'. self::$subCLName;
      $expLowBound = array( 'a' => 0 );
      $expUpBound  = array( 'a' => 100 );
      //actual value
      $actKey  = $mainInfo['ShardingKey'];
      $actType = $mainInfo['ShardingType'];
      $actMain = $mainInfo["IsMainCL"];
      $actSubCL    = $mainInfo["CataInfo"][0]["SubCLName"];
      $actLowBound = $mainInfo["CataInfo"][0]['LowBound'];
      $actUpBound  = $mainInfo["CataInfo"][0]['UpBound'];
      //compare results
      $this -> assertEquals( $expKey, $actKey ); 
      $this -> assertEquals( $expType, $actType ); 
      $this -> assertTrue( $actMain );  //expect IsMain: true
      $this -> assertEquals( $expSubCL, $actSubCL ); 
      $this -> assertEquals( $expLowBound, $actLowBound ); 
      $this -> assertEquals( $expUpBound, $actUpBound ); 
      
      //------------check results of subCL--------------------
      $subInfo = self::$dbh -> snapshotOper( self::$csName, self::$subCLName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //expect value
      $expKey    = array( 'a' => 1 );
      $expType   = 'hash';
      $expMainCL = self::$csName .'.'. self::$mainCLName;
      $expLowBound = array( '' => 0 );
      $expUpBound  = array( '' => 4096 );
      //actual value
      $actKey    = $subInfo['ShardingKey'];
      $actType   = $subInfo['ShardingType'];
      $actMainCL = $subInfo["MainCLName"];
      $actLowBound = $subInfo["CataInfo"][0]['LowBound'];
      $actUpBound  = $subInfo["CataInfo"][0]['UpBound'];
      //compare results
      $this -> assertEquals( $expKey, $actKey ); 
      $this -> assertEquals( $expType, $actType ); 
      $this -> assertEquals( $expMainCL, $actMainCL ); 
      $this -> assertEquals( $expLowBound, $actLowBound ); 
      $this -> assertEquals( $expUpBound, $actUpBound ); 
   }
   
   function test_detachCL()
   {
      echo "\n---Begin to detach cl.\n";
      
      $subFullName = self::$csName .'.'. self::$subCLName;
      self::$dbh -> detachCL( self::$mainCL, $subFullName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_snapshotForDetach()
   {
      echo "\n---Begin to check results by snapshot.\n";
      //when create cs by opthon, need to using snapshot and check attribute of options
      
      //----------check results of mainCL----------------
      $mainInfo = self::$dbh -> snapshotOper( self::$csName, self::$mainCLName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //compare results
      $this -> assertEquals( array( 'a' => 1 ), $mainInfo['ShardingKey']); 
      $this -> assertEquals( "range", $mainInfo['ShardingType'] ); 
      $this -> assertTrue( $mainInfo["IsMainCL"] );  //expect IsMain: true
      $this -> assertArrayNotHasKey( "SubCLName", $mainInfo["CataInfo"] ); 
      $this -> assertArrayNotHasKey( "LowBound",  $mainInfo["CataInfo"] ); 
      $this -> assertArrayNotHasKey( "UpBound",   $mainInfo["CataInfo"] ); 
      
      //------------check results of subCL--------------------
      $subInfo = self::$dbh -> snapshotOper( self::$csName, self::$subCLName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //compare results
      $this -> assertEquals( array( 'a' => 1 ), $subInfo['ShardingKey'] ); 
      $this -> assertEquals( "hash", $subInfo['ShardingType'] ); 
      $this -> assertArrayNotHasKey( "MainCLName", $subInfo ); 
      $this -> assertEquals( array( '' => 0 ),    $subInfo["CataInfo"][0]['LowBound'] ); 
      $this -> assertEquals( array( '' => 4096 ), $subInfo["CataInfo"][0]['UpBound'] ); 
   }
   
   function test_dropCL()
   {
      echo "\n---Begin to drop cl in the end.\n";
      //drop mainCL
      self::$dbh -> dropCL( self::$csName, self::$mainCLName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      //check mainCL
      self::$dbh -> getCL( self::$csDB, self::$mainCLName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -23, $errno );
      //drop mainCL
      self::$dbh -> dropCL( self::$csName, self::$subCLName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      //drop cs
      self::$dbh -> dropCS( self::$csName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno, 'Failed to drop cs, errCode:'. $errno );
   }
   
}
?>