/****************************************************
@description:      createCL / getCL / getFullName / getName / dropCS, only cover required parameter
@testlink cases:   seqDB-7652 / seqDB-7655
@input:        1 test_createCL, cover required parameter[$name]
               2 test_getCL
               3 test_alterCL, $options: array
               4 test_getFullName
               5 test_getName
               7 test_listCL, cover all param, $condition/$selector/$orderBy/$hint: null
                     -------[from doc]This parameter is reserved and must be null
               8 test_dropCL
@output:     success
@modify list:
        2016-4-18 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CLOperator02 extends BaseOperator 
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
      $condition = json_encode( array( 'Name' => $cl ) );
      $cursor = $this -> db -> snapshot( SDB_SNAP_CATALOG, $condition );
      return $cursor -> current();
   }
   
   function createCL( $csName, $clName )
   {
      $options = null;
      $this -> commCreateCL( $csName, $clName, $options, true );
   }
   
   function getCS( $csName )
   {
      return $this -> db -> getCS( $csName );
   } 
   
   function getCL( $csDB, $clName )
   {
      return $csDB -> getCL( $clName ); 
   } 
   
   function getFullName( $clDB )
   {
      return $clDB -> getFullName(); 
   }
   
   function getName( $clDB )
   {
      return $clDB -> getName(); 
   }
   
   function alterCL( $clDB )
   {
      $options = array( 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'hash' );
      $clDB -> alter( $options );
   }
   
   function listCL( $csName, $clName )
   {
      $cursor = $this -> db -> listCL();
      $cl = $csName .'.'. $clName;
      while( $tmpInfo = $cursor -> next() )
      {
         if( $tmpInfo = $cl )
         {
            return $tmpInfo;
         }
      }
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

class TestCL02 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $csDB;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CLOperator02();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME;
      
      echo "\n---Begin to drop cs/cl in the begin.\n";
      self::$dbh -> dropCS( self::$csName, true );
   }
   
   function test_createCL()
   {
      echo "\n---Begin to create cl.\n";
      
      self::$dbh -> createCL( self::$csName, self::$clName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_snapshotBeforeAlterOper()
   {
      echo "\n---Begin to check result by snapshot before alter.\n";
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         $clInfo = self::$dbh -> snapshotOper( self::$csName, self::$clName );
         
         $errno = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno );
         
         $this -> assertArrayNotHasKey( "ShardingKey", $clInfo );
         $this -> assertArrayNotHasKey( "ShardingType", $clInfo ); 
      }
   }
   
   function test_getCS()
   {
      echo "\n---Begin to get cs.\n";
      
      self::$csDB = self::$dbh -> getCS( self::$csName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertNotEmpty( self::$csDB );
   }
   
   function test_getCL()
   {
      echo "\n---Begin to get cl.\n";
      
      self::$clDB = self::$dbh -> getCL( self::$csDB, self::$clName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertNotEmpty( self::$clDB );
   }
   
   function test_alterCL()
   {
      echo "\n---Begin to alter cl.\n";
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         self::$dbh -> alterCL( self::$clDB );
         
         $errno = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno );
      }
   }
   
   function test_snapshotAfterAlterOper()
   {
      echo "\n---Begin to check result by snapshot after alter.\n";
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         $clInfo = self::$dbh -> snapshotOper( self::$csName, self::$clName );
         
         $errno = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno );
         
         $srdKey = $clInfo['ShardingKey'];
         $srdType = $clInfo['ShardingType'];
         $this -> assertEquals( array( 'a' => 1 ), $srdKey );
         $this -> assertEquals( 'hash', $srdType ); 
      }
   }
   
   function test_getFullName()
   {
      echo "\n---Begin to get fullName of the cl.\n";
      
      $fullName = self::$dbh -> getFullName( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $expFullName = self::$csName .'.'. self::$clName;
      $this -> assertEquals( $expFullName, $fullName );
   }
   
   function test_getName()
   {
      echo "\n---Begin to get name of the cl.\n";
      
      $name = self::$dbh -> getName( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertEquals( self::$clName, $name );
   }
   
   function test_listCL()
   {
      echo "\n---Begin to list cl.\n";
      
      $name = self::$dbh -> listCL( self::$csName, self::$clName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $expRecs = self::$csName .'.'. self::$clName;
      $this -> assertEquals( $expRecs, $name );
   }
   
   function test_dropCL()
   {
      echo "\n---Begin to drop cl in the end.\n";
      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare cs is not exist
      self::$dbh -> getCL( self::$csDB, self::$clName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -23, $errno );
   }
   
}
?>