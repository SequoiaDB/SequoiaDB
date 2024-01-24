/****************************************************
@description:      cl, setAttributes, nomal cl
@testlink cases:   seqDB-15159_01
@input:      setAttributes[ReplSize, ShardingKey, ShardingType, AutoSplit, EnsureShardingIndex, 
									Compressed, CompressionType, StrictDataMode]
@output:     fail: name
				 success
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator15159_01 extends BaseOperator 
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
   
   function createCS( $csName )
   {
      $csObj = $this -> db -> selectCS( $csName );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to createCS. Errno: ". $errno ."\n";
      }
		
		return $csObj;
   }
   
   function createCL( $csObj, $clName )
   {		
		$clObj = $csObj -> selectCL( $clName );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to createCL. Errno: ". $errno ."\n";
      }
		
		return $clObj;
   }
   
   function alterSetAttributes( $clObj, $options )
   {
      $clObj -> setAttributes( $options );
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
   
   function dropCL( $csName, $clName, $ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestCS15159_01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $csObj;
   private static $clName;
   private static $clObj;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15159_01].\n"; 
      self::$dbh = new CSOperator15159_01();
      
      if( self::$dbh -> commIsStandlone() === false )
		{      
			echo "   Begin to ready parameter.\n";
			self::$csName = self::$dbh -> COMMCSNAME;
			self::$clName = 'cl_15159_01';
         self::$groupNames = self::$dbh -> commGetGroupNames();
			
			echo "   Begin to drop cl in the begin.\n";
			self::$dbh -> dropCL( self::$csName, self::$clName, true );
			
			echo "   Begin to create cs[". self::$csName ."].\n"; 
			self::$csObj = self::$dbh -> createCS( self::$csName );
		
			echo "   Begin to create cl[". self::$clName ."].\n"; 
			self::$clObj = self::$dbh -> createCL( self::$csObj, self::$clName );
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
   
   function test_alterSetAttributes()
   {
      echo "\n---Begin to setAttributes, cover all valid attributes.\n"; 
      $options = '{ ReplSize:7, ShardingKey:{a:1}, ShardingType:"hash", Partition:2048, AutoSplit:true, EnsureShardingIndex:false, Compressed:true, CompressionType:"lzw", StrictDataMode:true }';
      self::$dbh -> alterSetAttributes( self::$clObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by snapshot.\n"; 
      $clInfo = self::$dbh -> snapshotCL( self::$csName.'.'.self::$clName );
		//var_dump($clInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName.'.'.self::$clName,  $clInfo["Name"] );		
      $this -> assertEquals( 7, $clInfo["ReplSize"] );
      $this -> assertEquals( 1, $clInfo["ShardingKey"]["a"] );
      $this -> assertEquals( "hash", $clInfo["ShardingType"] );
      $this -> assertEquals( 2048, $clInfo["Partition"] );
      $this -> assertTrue(  $clInfo["AutoSplit"] );
      $this -> assertFalse( $clInfo["EnsureShardingIndex"] );
      $this -> assertEquals( "Compressed | StrictDataMode", $clInfo["AttributeDesc"] );
      $this -> assertEquals( 1, $clInfo["CompressionType"] );
      $this -> assertEquals( "lzw", $clInfo["CompressionTypeDesc"] );
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to clean env.\n"; 
      echo "   Begin to drop cl in the end.\n"; 
		self::$dbh -> dropCL( self::$csName, self::$clName, false );
		$errno = self::$dbh -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to dropCS. Errno: ". $errno ."\n";
      }
   }
   
}
?>