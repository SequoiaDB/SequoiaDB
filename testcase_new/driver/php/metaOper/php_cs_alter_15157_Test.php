/****************************************************
@description:      cs, alter
@testlink cases:   seqDB-15157
@input:      1. setDomain
				 2. removeDomain
@output:     fail: name
				 success: PageSise, LobPageSize
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator15157 extends BaseOperator 
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
   
   function createCL( $csObj, $clName, $options )
   {		
		$csObj -> createCL( $clName, $options );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to createCL. Errno: ". $errno ."\n";
      }
   }
   
   function alterSetDomain( $csObj, $options )
   {
      $csObj -> setDomain( $options );
   } 
   
   function alterRemoveDomain( $csObj )
   {
      $csObj -> removeDomain();
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

class TestCS15157 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $cataObj;
   private static $dmName1;
   private static $dmName2;
   private static $dmName3;
   private static $csName;
   private static $csObj;
   private static $clName;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15157].\n"; 
      self::$dbh = new CSOperator15157();
      
      if( self::$dbh -> commIsStandlone() === false )
		{      
			echo "   Begin to ready parameter.\n";
			self::$dmName1 = self::$dbh -> COMMDOMAINNAME .'_15157_01';
			self::$dmName2 = self::$dbh -> COMMDOMAINNAME .'_15157_02';
			self::$dmName3 = self::$dbh -> COMMDOMAINNAME .'_15157_03';
			self::$csName = self::$dbh -> COMMCSNAME.'_15157';
			self::$clName = 'cl_15157';
         self::$groupNames = self::$dbh -> commGetGroupNames();
			self::$cataObj = self::$dbh -> connectCatalog();
			
			echo "   Begin to drop domain/cs in the begin.\n";
			self::$dbh -> dropCS( self::$csName,  true );
			self::$dbh -> dropDM( self::$dmName1, true );
			self::$dbh -> dropDM( self::$dmName2, true );
			self::$dbh -> dropDM( self::$dmName3, true );
			
			echo "   Begin to create domain[". self::$dmName1 .", ". self::$dmName2 .", ". self::$dmName3 ."].\n";
			$options1 = array( 'Groups' => array( self::$groupNames[0], self::$groupNames[1] ) );
			$options2 = array( 'Groups' => array( self::$groupNames[0] ) );
			$options3 = array( 'Groups' => array( self::$groupNames[1] ) );
			self::$dbh -> createDM( self::$dmName1, $options1 );
			self::$dbh -> createDM( self::$dmName2, $options2 );
			self::$dbh -> createDM( self::$dmName3, $options3 );
			
			echo "   Begin to create cs[". self::$csName ."].\n"; 
			self::$csObj = self::$dbh -> createCS( self::$csName );		
		
			echo "   Begin to create cl[". self::$clName ."].\n"; 
			$options = array( 'Group' => self::$groupNames[0] ); 
			self::$dbh -> createCL( self::$csObj, self::$clName, $options );
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
   
   function test_alterSetDomain01()
   {
      echo "\n---Begin to setDomain, add domain.\n"; 
      $options = array( 'Domain' => self::$dmName1 );
      self::$dbh -> alterSetDomain( self::$csObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by find[SYSCAT.SYSCOLLECTIONSPACES].\n"; 
      $csInfo = self::$dbh -> checkResults( self::$cataObj, self::$csName );
		//var_dump($csInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName,  $csInfo['Name'] );
      $this -> assertEquals( self::$dmName1, $csInfo['Domain'] );
   }
   
   function test_alterSetDomain02()
   {
      echo "\n---Begin to setDomain, modify domain, newDomain contains group of cl.\n"; 
      $options = array( 'Domain' => self::$dmName2 );
      self::$dbh -> alterSetDomain( self::$csObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by find[SYSCAT.SYSCOLLECTIONSPACES of catalog].\n"; 
      $csInfo = self::$dbh -> checkResults( self::$cataObj, self::$csName );
		//var_dump($csInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName,  $csInfo['Name'] );
      $this -> assertEquals( self::$dmName2, $csInfo['Domain'] );
   }
   
   function test_alterSetDomain03()
   {
      echo "\n---Begin to setDomain, modify domain, newDomain not contains group of cl.\n"; 
      $options = array( 'Domain' => self::$dmName3 );
      self::$dbh -> alterSetDomain( self::$csObj, $options );
      $this -> assertEquals( -216, self::$dbh -> getErrno() );
   }
   
   function test_alterRemoveDomain()
   {
      echo "\n---Begin to removeDomain.\n"; 
      self::$dbh -> alterRemoveDomain( self::$csObj );
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
		
      echo "   Begin to check results by find[SYSCAT.SYSCOLLECTIONSPACES of catalog].\n"; 
      $csInfo = self::$dbh -> checkResults( self::$cataObj, self::$csName );
		//var_dump($csInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( self::$csName,  $csInfo['Name'] );
      $this -> assertArrayNotHasKey( 'Domain', $csInfo );
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
		
      self::$dbh -> dropDM( self::$dmName3, false );
		$errno = self::$dbh -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to dropDomain. Errno: ". $errno ."\n";
      }
   }
   
}
?>