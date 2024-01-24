/****************************************************
@description:      cl, alter
@testlink cases:   seqDB-15160
@input:      1. enableSharding
				 2. disableSharding
@output:     success
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator15160 extends BaseOperator 
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
   
   function alterEnableSharding( $clObj, $options )
   {
      $clObj -> enableSharding( $options );
   } 
   
   function alterDisableSharding( $clObj )
   {
      $clObj -> disableSharding();
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

class TestCS15160 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $csObj;
   private static $clName;
   private static $clObj;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15160].\n"; 
      self::$dbh = new CSOperator15160();
      
      if( self::$dbh -> commIsStandlone() === false )
		{      
			echo "   Begin to ready parameter.\n";
			self::$csName = self::$dbh -> COMMCSNAME;
			self::$clName = 'cl_15160';
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
   
   function test_alterEnableSharding01()
   {
      echo "\n---Begin to enableSharding[...AutoSplit:false...].\n"; 
      $options = '{ ShardingKey:{a:1}, ShardingType:"hash", Partition:2048, AutoSplit:false, EnsureShardingIndex:false }';
      self::$dbh -> alterEnableSharding( self::$clObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by snapshot.\n"; 
      $clInfo = self::$dbh -> snapshotCL( self::$csName.'.'.self::$clName );
		//var_dump($clInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName.'.'.self::$clName,  $clInfo['Name'] );
      $this -> assertEquals( 1, $clInfo["ShardingKey"]["a"] );
      $this -> assertEquals( "hash", $clInfo["ShardingType"] );
      $this -> assertEquals( 2048, $clInfo["Partition"] );
      $this -> assertFalse(  $clInfo["AutoSplit"] );
      $this -> assertFalse( $clInfo["EnsureShardingIndex"] );
   }
   
   function test_alterDisableSharding01()
   {
      echo "\n---Begin to disableSharding[after AutoSplit:false].\n";
      self::$dbh -> alterDisableSharding( self::$clObj );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by snapshot.\n"; 
      $clInfo = self::$dbh -> snapshotCL( self::$csName.'.'.self::$clName );
		//var_dump($clInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName.'.'.self::$clName,  $clInfo['Name'] );
      $this -> assertArrayNotHasKey( 'ShardingKey', $clInfo );
      $this -> assertArrayNotHasKey( 'ShardingType', $clInfo );
      $this -> assertArrayNotHasKey( 'Partition', $clInfo );
      $this -> assertArrayNotHasKey( 'AutoSplit', $clInfo );
      $this -> assertArrayNotHasKey( 'EnsureShardingIndex', $clInfo );
   }
   
   function test_alterEnableSharding02()
   {
      echo "\n---Begin to enableSharding[...AutoSplit:true...].\n"; 
      $options = '{ ShardingKey:{a:-1}, ShardingType:"hash", Partition:1024, AutoSplit:true, EnsureShardingIndex:true }';
      self::$dbh -> alterEnableSharding( self::$clObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by snapshot.\n"; 
      $clInfo = self::$dbh -> snapshotCL( self::$csName.'.'.self::$clName );
		//var_dump($clInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName.'.'.self::$clName,  $clInfo['Name'] );
      $this -> assertEquals( -1, $clInfo["ShardingKey"]["a"] );
      $this -> assertEquals( "hash", $clInfo["ShardingType"] );
      $this -> assertEquals( 1024, $clInfo["Partition"] );
      $this -> assertTrue(  $clInfo["AutoSplit"] );
      $this -> assertTrue( $clInfo["EnsureShardingIndex"] );
      $this -> assertEquals( count( self::$groupNames ), count( $clInfo["CataInfo"] ) );
   }
   
   function test_alterDisableSharding02()
   {
      echo "\n---Begin to disableSharding[after AutoSplit:true].\n";
      self::$dbh -> alterDisableSharding( self::$clObj );
      $this -> assertEquals( -32, self::$dbh -> getErrno() );
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
