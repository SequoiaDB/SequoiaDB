/****************************************************
@description:      selectCL , base case
      selecCL: Get the specified collection space, if is not exist,will auto create.
@testlink cases:   seqDB-7653 / seqDB-7655
@input:        1 test_selectCL, 
                     when cs is not exist, selectCL, parameter cover:
                           1) only cover required parameter[$name]
                           2) cover all parameter, $options: array
                           3) cover all parameter, $options: string
                     when cs is exist, selectCL.
               2 test_dropCL
@output:     success
@modify list:
        2016-4-15 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CLOperator03 extends BaseOperator 
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
   
   function selectCS( $csName )
   {
      return $this -> db -> selectCS( $csName );
   }
   
   function selectCL( $csDB, $clName, $options = null )
   {
      return $csDB -> selectCL( $clName, $options );
   }
   
   function getCL( $csName, $clName )
   {
      $cl = $csName .'.'. $clName;
      return $this -> db -> getCL( $cl );
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

class TestCL03 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $csDB;
   private static $clName1;
   private static $clName2;
   private static $clName3;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CLOperator03();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName1 = self::$dbh -> COMMCLNAME .'_1';
      self::$clName2 = self::$dbh -> COMMCLNAME .'_2';
      self::$clName3 = self::$dbh -> COMMCLNAME .'_3';
      
      echo "\n---Begin to drop cs/cl in the begin.\n";
      self::$dbh -> dropCS( self::$csName, true );
   }
   
   function test_selectCS()
   {
      echo "\n---Begin to select cs.\n";
      
      self::$csDB = self::$dbh -> selectCS( self::$csName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_selectCLByNotExist()
   {
      echo "\n---Begin to select cl when cl is not exist.\n";
      
      //the cl is not exist, only cover required param
      self::$dbh -> selectCL( self::$csDB, self::$clName1 );
      $errno1 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno1 ); 
      
      //the cl is not exist, $options: array
      $options2 = array( 'ReplSize' => 1 );
      self::$dbh -> selectCL( self::$csDB, self::$clName2, $options2 );
      $errno2 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno2 );
      
      //the cl is not exist, $options: string
      $options3 = json_encode( array( 'ReplSize' => 1 ) );
      self::$dbh -> selectCL( self::$csDB, self::$clName3, $options3 );
      $errno3 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno3 );
   }
   
   function test_getCL()
   {
      echo "\n---Begin to get cl.\n";
      
      $clInfo1 = self::$dbh -> getCL( self::$csName, self::$clName1 );
      $errno1  = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno1 );
      $this -> assertNotEmpty( $clInfo1 );
   }
   
   function test_selectCLByExist()
   {
      echo "\n---Begin to select cl when cl is exist.\n";
      $options = array( 'ReplSize' => 1 );
      self::$dbh -> selectCL( self::$csDB, self::$clName1, $options );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno, 
                  'Failed to select cl['. self::$csName .'.'. self::$clName1 .'], errCode:'. $errno ); 
   }
   
   function test_snapshot()
   {
      echo "\n---Begin to check results by snapshot.\n";
      if( self::$dbh -> commIsStandlone() === false )
      {
         //when create cs by opthon, need to using snapshot and check attribute of options
         
         //$clName1
         $clInfo1 = self::$dbh -> snapshotOper( self::$csName, self::$clName1 );
         $errno1 = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno1 ); 
         
         $expName1 = self::$csName .'.'. self::$clName1;
         $this -> assertEquals( $expName1, $clInfo1['Name'] );
         $this -> assertEquals( 1,  $clInfo1["Attribute"] );
         $this -> assertEquals( "Compressed", $clInfo1["AttributeDesc"] );
         $this -> assertArrayNotHasKey( "ReplSize", $clInfo1 );
         
         //$clName2, $options = array( 'ReplSize' => 1 )
         $clInfo2 = self::$dbh -> snapshotOper( self::$csName, self::$clName2 );
         $errno2 = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno2 );
         
         $expName2 = self::$csName .'.'. self::$clName2;
         $this -> assertEquals( $expName2, $clInfo2['Name'] );
         $this -> assertEquals( 1, $clInfo2['ReplSize'] );
         
         //$csName3, $options = json_encode( array( 'ReplSize' => 1 ) )
         $clInfo3 = self::$dbh -> snapshotOper( self::$csName, self::$clName3 );
         $errno3 = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno3 );
         
         $expName3 = self::$csName .'.'. self::$clName3;
         $this -> assertEquals( $expName3, $clInfo3['Name'] );
         $this -> assertEquals( 1, $clInfo3['ReplSize'] );
      }
   }
   
   function test_dropCL()
   {
      echo "\n---Begin to drop cl in the end.\n";
      
      self::$dbh -> dropCL( self::$csName, self::$clName1, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      //compare cl is not exist
      self::$dbh -> getCL( self::$csName, self::$clName1 );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -23, $errno );
      
      self::$dbh -> dropCL( self::$csName, self::$clName2, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      self::$dbh -> dropCL( self::$csName, self::$clName3, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
}
?>