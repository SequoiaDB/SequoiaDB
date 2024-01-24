/****************************************************
@description:      cs, alter, batch modify attributes
@testlink cases:   seqDB-15156
@input:        alter[Name, PageSise, LobPageSize]
@output:     fail: name
				 success: PageSise, LobPageSize
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator15156_02 extends BaseOperator 
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
   
   function createDM( $dmName, $options )
   {
      $this -> commCreateDomain( $dmName, $options );
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
   
   function createCL( $csObj, $clName )
   {		
		$csObj -> createCL( $clName );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to createCL. Errno: ". $errno ."\n";
      }
   }
   
   function alterCS( $csObj, $options )
   {
      $csObj -> alter( $options );
   } 
	
	function checkResults( $cataObj, $csName ) {
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
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
   function dropDM( $dmName, $ignoreNotExist )
   {
      $this -> commDropDomain( $dmName, $ignoreNotExist );
   }
   
}

class TestCS15156_02 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $cataObj;
   private static $dmName1;
   private static $dmName2;
   private static $csName;
   private static $csObj;
   private static $clName;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15156_02].\n"; 
      self::$dbh = new CSOperator15156_02();
      
      if( self::$dbh -> commIsStandlone() === false )
		{      
			echo "   Begin to ready parameter.\n";
			self::$dmName1 = self::$dbh -> COMMDOMAINNAME .'_15156_02_01';
			self::$dmName2 = self::$dbh -> COMMDOMAINNAME .'_15156_02_02';
			self::$csName = self::$dbh -> COMMCSNAME.'_15156_02';
			self::$clName = 'cl_15156_02';
         self::$groupNames = self::$dbh -> commGetGroupNames();
			self::$cataObj = self::$dbh -> connectCatalog();
			
			echo "   Begin to drop domain/cs in the begin.\n";
			self::$dbh -> dropCS( self::$csName, true );
			self::$dbh -> dropDM( self::$dmName1, true );
			self::$dbh -> dropDM( self::$dmName2, true );
			
			echo "   Begin to create domain[". self::$dmName1 .", ". self::$dmName2 ."].\n";
			$options = array( 'Groups' => array( self::$groupNames[0] ) );
			self::$dbh -> createDM( self::$dmName1, $options );
			self::$dbh -> createDM( self::$dmName2, $options );
			
			echo "   Begin to create cs[". self::$csName ."].\n"; 
			$options = array( 'Domain' => self::$dmName1, 'PageSize' => 8192, 'LobPageSize' => 8192 ); 
			self::$csObj = self::$dbh -> createCS( self::$csName, $options );
		}
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_alterCS01()
   {
      echo "\n---Begin to alter[batch modify attributes, ignoreE is true].\n"; 
		$options = '{ Alter:[ {Name:"set attributes", Args:{Domain:"'.self::$dmName2.'", PageSize:4096}}, {Name:"set attributes", Args:{Name:"test15156_02"}}, {Name:"set attributes", Args:{LobPageSize:4096}} ], Options:{IgnoreException:true} }'; //true
      self::$dbh -> alterCS( self::$csObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
		echo "   Begin to create cl[". self::$clName ."].\n"; 
      self::$dbh -> createCL( self::$csObj, self::$clName );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );	
		
      echo "   Begin to check results by find[SYSCAT.SYSCOLLECTIONSPACES].\n"; 
      $csInfo = self::$dbh -> checkResults( self::$cataObj, self::$csName );
		//var_dump($csInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName,  $csInfo['Name'] );
      $this -> assertEquals( self::$dmName2, $csInfo['Domain'] );
      $this -> assertEquals( 4096, $csInfo['PageSize'] );
      $this -> assertEquals( 4096, $csInfo['LobPageSize'] );
   }
   
   function test_alterCS02()
   {
      echo "\n---Begin to alter[batch modify attributes, ignoreE is false].\n"; 
		$options = '{ Alter:[ {Name:"set attributes", Args:{Domain:"'.self::$dmName2.'", PageSize:4096}}, {Name:"set attributes", Args:{Test:"test15156_02"}}, {Name:"set attributes", Args:{LobPageSize:4096}} ], Options:{IgnoreException:false} }'; //false
      self::$dbh -> alterCS( self::$csObj, $options );
      $this -> assertEquals( -275, self::$dbh -> getErrno() );
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
		
      echo "   Begin to drop domain in the end.\n"; 
      self::$dbh -> dropDM( self::$dmName1, false );
		$errno = self::$dbh -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to dropDomain. Errno: ". $errno ."\n";
      }
		
      self::$dbh -> dropDM( self::$dmName2, false );
		$errno = self::$dbh -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to dropDomain. Errno: ". $errno ."\n";
      }
   }
   
}
?>