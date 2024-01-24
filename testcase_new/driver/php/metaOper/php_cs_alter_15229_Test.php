/****************************************************
@description:      cs, enableCapped/disableCapped, capped cs
@testlink cases:   seqDB-15229
@input:      1. enableCapped
				 2. disableCapped
@output:     success
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator15229 extends BaseOperator 
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
	
	function connectCatalog(  ) 
	{
		return $this -> commConnectCatalog();
	}
   
   function createCS( $csName )
   {
      $this -> db -> createCS( $csName );
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
   
   function alterEnableCapped( $csObj )
   {
      $csObj -> enableCapped();
   } 
   
   function alterdisableCapped( $csObj )
   {
      $csObj -> disableCapped();
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
	
	function checkResultsForCS( $cataObj, $csName ) {
		$csObj = $cataObj -> getCS( 'SYSCAT' );
		$errno = $cataObj -> getError()['errno'];
      if( $errno !== 0 )
      {
         echo "\nFailed to get cs. Errno: ". $errno ."\n";
      }
		
		$clObj = $csObj -> getCL( 'SYSCOLLECTIONSPACES' );
		$errno = $cataObj -> getError()['errno'];
      if( $errno !== 0 )
      {
         echo "\nFailed to get cl. Errno: ". $errno ."\n";
      }
		
		$condition = array( 'Name' => $csName );
		$cursor = $clObj -> find( $condition );
		$errno = $cataObj -> getError()['errno'];
      if( $errno !== 0 )
      {
         echo "\nFailed to find records. Errno: ". $errno ."\n";
      }
		return $cursor -> current();
	}
   
   function checkResultsForCL( $fulClName )
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
   
   function dropCL( $csName, $clName, $ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestCS15229 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $cataObj;
   private static $csName;
   private static $csObj;
   private static $clName;
   private static $clObj;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15229].\n"; 
      self::$dbh = new CSOperator15229();
      
      if( self::$dbh -> commIsStandlone() === false )
		{      
			echo "   Begin to ready parameter.\n";
			self::$csName = self::$dbh -> COMMCSNAME."_15229";
			self::$clName = 'cl_15229';
         self::$groupNames = self::$dbh -> commGetGroupNames();
			self::$cataObj = self::$dbh -> connectCatalog();
			
			echo "   Begin to drop cs in the begin.\n";
			self::$dbh -> dropCS( self::$csName,  true );
			
			echo "   Begin to create cs[". self::$csName ."].\n"; 
			self::$csObj = self::$dbh -> createCS( self::$csName );
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
   
   function test_alterEnableCapped()
   {
      echo "\n---Begin to enableCapped.\n"; 
      self::$dbh -> alterEnableCapped( self::$csObj );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results for cs by find[SYSCAT.SYSCOLLECTIONSPACES].\n"; 
      $csInfo = self::$dbh -> checkResultsForCS( self::$cataObj, self::$csName );
		//var_dump($csInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName,  $csInfo['Name'] );
      $this -> assertEquals( 1,  $csInfo['Type'] );
		
		echo "   Begin to create cl[". self::$clName ."].\n"; 
		$options = '{ Capped:true, Size:2048, Max:10000, OverWrite:false, AutoIndexId:false }';
		self::$clObj = self::$dbh -> createCL( self::$csObj, self::$clName, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results for cl by snapshot.\n"; 
      $clInfo = self::$dbh -> checkResultsForCL( self::$csName.'.'.self::$clName );
		//var_dump($clInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName.'.'.self::$clName,  $clInfo['Name'] );
      if( is_object( $clInfo["Size"] ) )
      {
         $this -> assertEquals( "2147483648", $clInfo["Size"] -> __toString() );
         $this -> assertEquals( "10000", $clInfo["Max"] -> __toString() );
      }
      else{
         $this -> assertEquals( "2147483648", $clInfo["Size"] );
         $this -> assertEquals( "10000", $clInfo["Max"] );
      }
      $this -> assertFalse(  $clInfo["OverWrite"] );
   }
   
   function test_alterDisableCapped01()
   {
      echo "\n---Begin to enableCapped.\n"; 
      self::$dbh -> alterDisableCapped( self::$csObj );
      $this -> assertEquals( -275, self::$dbh -> getErrno() );
   }
   
   function test_alterDisableCapped02()
   {		
		echo "\n---Begin to drop cl in the begin.\n";
		self::$dbh -> dropCL( self::$csName, self::$clName, false );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to enableCapped.\n"; 
      self::$dbh -> alterDisableCapped( self::$csObj );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by find[SYSCAT.SYSCOLLECTIONSPACES].\n"; 
      $csInfo = self::$dbh -> checkResultsForCS( self::$cataObj, self::$csName );
		//var_dump($csInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName,  $csInfo['Name'] );
      $this -> assertEquals( 0,  $csInfo['Type'] );
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