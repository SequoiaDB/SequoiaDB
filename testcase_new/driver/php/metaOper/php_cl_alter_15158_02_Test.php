/****************************************************
@description:      cl, alter, capped cl
@testlink cases:   seqDB-15158_02
@input:      alter[Size, Max, OverWrite]
@output:     success
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator15158_02 extends BaseOperator 
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
   
   function createCS( $csName, $options )
   {
      $this -> db -> createCS( $csName, $options );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to createCS. Errno: ". $errno ."\n";
      }
		
		$csObj = $this -> db -> getCS( $csName );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to getCS. Errno: ". $errno ."\n";
      }
		
		return $csObj;
   }
   
   function createCL( $csObj, $clName, $options )
   {		
		$clObj = $csObj -> selectCL( $clName , $options);
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to createCL. Errno: ". $errno ."\n";
      }
		
		return $clObj;
   }
   
   function alterCL( $clObj, $options )
   {
      $clObj -> alter( $options );
   } 
   
   function snapshotCL( $fulClName )
   {
      $condition = array( 'Name' => $fulClName ); 
      $cursor = $this -> db -> snapshot( SDB_SNAP_CATALOG, $condition );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to listCS. Errno: ". $errno ."\n";
      }
      return $cursor -> current();
   } 
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
}

class TestCS15158_02 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $csObj;
   private static $clName;
   private static $clObj;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15158_02].\n"; 
      self::$dbh = new CSOperator15158_02();
      
      if( self::$dbh -> commIsStandlone() === false )
		{      
			echo "   Begin to ready parameter.\n";
			self::$csName = self::$dbh -> COMMCSNAME."_15158_02";
			self::$clName = 'cl_15158_02';
         self::$groupNames = self::$dbh -> commGetGroupNames();
			
			echo "   Begin to drop cs in the begin.\n";
			self::$dbh -> dropCS( self::$csName,  true );
			
			echo "   Begin to create cs[". self::$csName ."].\n"; 
			$options = '{ Capped:true }';
			self::$csObj = self::$dbh -> createCS( self::$csName, $options );
		
			echo "   Begin to create cl[". self::$clName ."].\n"; 
			$options = '{ Capped:true, Size:1024, Max:10000, OverWrite:false, AutoIndexId:false }';
			self::$clObj = self::$dbh -> createCL( self::$csObj, self::$clName, $options );
		}
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
      else if( count(self::$groupNames) < 2 )
      {
         $this -> markTestSkipped( "GroupNumber < 2." );
      }
   }
   
   function test_alterCL()
   {
      echo "\n---Begin to alter, cover all valid attributes.\n"; 
      $options = '{ Size:2048, Max:20000, OverWrite:true }';
      self::$dbh -> alterCL( self::$clObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by snapshot.\n"; 
      $clInfo = self::$dbh -> snapshotCL( self::$csName.'.'.self::$clName );
		//var_dump($clInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName.'.'.self::$clName,  $clInfo["Name"] );
      if( is_object( $clInfo["Size"] ) )
      {
         $this -> assertEquals( "2147483648", $clInfo["Size"] -> __toString() );
         $this -> assertEquals( "20000", $clInfo["Max"] -> __toString() );
      }
      else{
         $this -> assertEquals( "2147483648", $clInfo["Size"] );
         $this -> assertEquals( "20000", $clInfo["Max"] );
      }
      $this -> assertTrue(  $clInfo["OverWrite"] );
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to clean env.\n";
      echo "   Begin to drop cs in the end.\n"; 
      self::$dbh -> dropCS( self::$csName, false );
		$errno = self::$dbh -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to dropCS. Errno: ". $errno ."\n";
      } 
   }
   
}
?>