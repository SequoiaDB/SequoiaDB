/****************************************************
@description:      selectCS / getCS /dropCS, base case
      selecCS: Get the specified collection space, if is not exist,will auto create.
@testlink cases:   seqDB-7649
@input:        1 test_selectCS, 
                     when cs is not exist, selectCS, parameter cover:
                           1) only cover required parameter[$name]
                           2) cover all parameter, $options: array
                           3) cover all parameter, $options: string
                     when cs is exist, selectCS.
               2 test_getCS
               3 test_dropCS
@output:     success
@modify list:
        2016-4-15 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator03 extends BaseOperator 
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
      $condition = array( 'Name' => $csName );
      $cursor = $this -> db -> snapshot( SDB_SNAP_COLLECTIONSPACE, $condition );
      
      return $cursor -> current();
   }
   
   function selectCS( $csName, $options = null )
   {
      return $this -> db -> selectCS( $csName, $options );
   }
   
   //get csInfo by snapshot need cl, else snapshot result is null
   function createCL( $csName, $clName )
   {
      $this -> commCreateCL( $csName, $clName );
   }
   
   function listCS( $csName )
   {
      $cursor = $this -> db -> listCS();
      while( $tmpInfo = $cursor -> next() )
      {
         if ( $tmpInfo['Name'] === $csName )
         {
            return $tmpInfo['Name'];
         }
      }
      return false;
   } 
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
}

class TestCS03 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName1;
   private static $csName2;
   private static $csName3;
   private static $clName;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CSOperator03();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName1 = self::$dbh -> COMMCSNAME .'_1';
      self::$csName2 = self::$dbh -> COMMCSNAME .'_2';
      self::$csName3 = self::$dbh -> COMMCSNAME .'_3';
      self::$clName  = self::$dbh -> COMMCLNAME;
      
      echo "\n---Begin to drop cs in the begin.\n";
      self::$dbh -> dropCS( self::$csName1, true );
      self::$dbh -> dropCS( self::$csName2, true );
      self::$dbh -> dropCS( self::$csName3, true );
   }
   
   function test_selectCSByNotExist()
   {
      echo "\n---Begin to select cs when cs is not exist.\n";
      
      //the cs is not exist, only cover required param
      self::$dbh -> selectCS( self::$csName1 );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //the cs is not exist, $options: array
      $options = array( 'PageSize' => 0 );
      self::$dbh -> selectCS( self::$csName2, $options );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      //the cs is not exist, $options: string
      $options = json_encode( array( 'PageSize' => 4096 ) );
      self::$dbh -> selectCS( self::$csName3, $options );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_createCL()
   {
      echo "\n---Begin to create cl for snapshot.\n";
      //when create cs by opthon, need to create cl in the cs, because need cl to snapshot and check attribute of options
      
      self::$dbh -> createCL( self::$csName2, self::$clName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      self::$dbh -> createCL( self::$csName3, self::$clName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_snapshot()
   {
      echo "\n---Begin to check results by snapshot.\n";
      //when create cs by opthon, need to using snapshot and check attribute of options
      
      //$csName2, $options = array( 'PageSize' => 0 )
      $csInfo2 = self::$dbh -> snapshotOper( self::$csName2 );
      
      $errno2 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno2 ); 
      
      $name2 = $csInfo2['Name'];
      $pageSize2 = $csInfo2['PageSize'];
      $this -> assertEquals( self::$csName2, $name2 );
      $this -> assertEquals( 65536, $pageSize2 );
      
      //$csName3, $options = json_encode( array( 'PageSize' => 4096 ) )
      $csInfo3 = self::$dbh -> snapshotOper( self::$csName3 );
      
      $errno3 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno3 );
      
      $name3 = $csInfo3['Name'];
      $pageSize3 = $csInfo3['PageSize'];
      $this -> assertEquals( self::$csName3, $name3 );
      $this -> assertEquals( 4096, $pageSize3 );
   }
   
   function test_selectCSByExist()
   {
      echo "\n---Begin to select cs when cs is exist.\n";
      
      $tmpInfo = self::$dbh -> selectCS( self::$csName1 );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      $this -> assertNotEmpty( $tmpInfo );
   }
   
   function test_dropCS()
   {
      echo "\n---Begin to drop cs in the end.\n";
      
      self::$dbh -> dropCS( self::$csName1, false );
      $errno1 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno1 );
      
      self::$dbh -> dropCS( self::$csName2, false );
      $errno2 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno2 );
      
      self::$dbh -> dropCS( self::$csName3, false );
      $errno3 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno3 );
   }
   
}
?>