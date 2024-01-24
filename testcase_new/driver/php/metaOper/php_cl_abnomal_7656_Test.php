/****************************************************
@description:      attachCL, duplicated attach collection partition
@testlink cases:   seqDB-7654
@input:        1 test_createCS/test_createCL
               2 test_getCS/test_getCL, duplicated attach collection partition
               3 test_attachCL, the same range
               4 test_snapshotForAttach
               5 test_dropCL/test_dropCS
@output:     Errno: -237
@modify list:
        2016-4-20 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CLOperator01 extends BaseOperator 
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
      $options = array( 'LowBound' => array( 'a' => 0 ), 'UpBound' => array( 'a' => 100 ) );
      
      $mainCL -> attachCL( $subFullName, $options );
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

class TestCL01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $mainCLName;
   private static $subCLName1;
   private static $subCLName2;
   private static $csDB;
   private static $mainCL;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CLOperator01();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$mainCLName = self::$dbh -> COMMCLNAME .'_mainCL';
      self::$subCLName1  = self::$dbh -> COMMCLNAME .'_subCL_1';
      self::$subCLName2  = self::$dbh -> COMMCLNAME .'_subCL_2';
      
      echo "\n---Begin to drop cs/cl in the begin.\n";
      self::$dbh -> dropCS( self::$csName, true );
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
      
      //subCL1
      self::$dbh -> createCL( self::$csName, self::$subCLName1, 'subCL' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //subCL2
      self::$dbh -> createCL( self::$csName, self::$subCLName2, 'subCL' );
      
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
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         //attach $subFullName1
         $subFullName1 = self::$csName .'.'. self::$subCLName1;
         self::$dbh -> attachCL( self::$mainCL, $subFullName1 );
         $errno = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno ); 
         
         //attach $subFullName2
         $subFullName2 = self::$csName .'.'. self::$subCLName2;
         self::$dbh -> attachCL( self::$mainCL, $subFullName2 );
         $errno = self::$dbh -> getErrno();
         $this -> assertEquals( -237, $errno ); 
      }
   }
   
   function test_snapshotForAttach()
   {
      echo "\n---Begin to check results by snapshot.\n";
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         //when create cs by opthon, need to using snapshot and check attribute of options
         
         //----------check results of mainCL----------------
         $mainInfo = self::$dbh -> snapshotOper( self::$csName, self::$mainCLName );
         
         $errno = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno ); 
         
         $expSubCL = self::$csName .'.'. self::$subCLName1;
         $expNum   = 1;
         $actSubCL = $mainInfo["CataInfo"][0]["SubCLName"];
         $actNum   = count( $mainInfo["CataInfo"] );
         $this -> assertEquals( $expSubCL, $actSubCL ); 
         $this -> assertEquals( $expNum, $actNum ); 
      }
   }
   
   function test_dropCL()
   {
      echo "\n---Begin to drop cs in the end.\n";
      
      self::$dbh -> dropCS( self::$csName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno, 'Failed to drop cs, errCode:'. $errno );
   }
   
}
?>