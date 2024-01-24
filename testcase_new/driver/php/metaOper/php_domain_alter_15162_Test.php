/****************************************************
@description:      alter, addGroups / setGroups / removeGroups
@testlink cases:   seqDB-15162
@input:      1. addGroups
				 2. removeGroups
				 3. setGroups
@output:     success
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DomainOper15162 extends BaseOperator 
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
   
   function createDM( $dmName, $options )
   {
      $this -> commCreateDomain( $dmName, $options );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to createDomain. Errno: ". $errno ."\n";
      }
		
      $dmObj = $this -> db -> getDomain( $dmName );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to getDomain. Errno: ". $errno ."\n";
      }
		
		return $dmObj;
   }
   
   function alterAddGroups( $dmObj, $options )
   {
      $dmObj -> addGroups( $options );
   }
   
   function alterSetGroups( $dmObj, $options )
   {
      $dmObj -> setGroups( $options );
   }
   
   function alterRemoveGroups( $dmObj, $options )
   {
      $dmObj -> removeGroups( $options );
   }
   
   function listDM( $dmName )
   {
      $condition = array( 'Name' => $dmName );      
      $cursor = $this -> db -> listDomain( $condition );
		$errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to list domain. Errno: ". $errno ."\n";
      }
      return $cursor -> current();
   } 
   
   function dropDM( $dmName, $ignoreNotExist )
   {
      $this -> commDropDomain( $dmName, $ignoreNotExist );
   }
   
}

class TestDomain15162 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $dmName;
   private static $dmObj;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15162].\n"; 
      self::$dbh = new DomainOper15162();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "   Begin to ready parameter.\n";
         self::$dmName = self::$dbh -> COMMDOMAINNAME .'_15162';
         self::$groupNames = self::$dbh -> commGetGroupNames();
         
         echo "   Begin to drop domain in the begin.\n";
         self::$dbh -> dropDM( self::$dmName, true );
						
			echo "   Begin to create domain[". self::$dmName ."].\n";
			$options = array( 'Groups' => array( self::$groupNames[0] ), 'AutoSplit' => false );
			self::$dmObj = self::$dbh -> createDM( self::$dmName,  $options );
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
   
   function test_alterAddGroups()
   {
      echo "\n---Begin to alter[addGroups].\n"; 
      $options = array( 'Groups' => array( self::$groupNames[1] ) );
      self::$dbh -> alterAddGroups( self::$dmObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() ); 
		
      echo "   Begin to check results.\n";
      $dmObjInfo = self::$dbh -> listDM( self::$dmName );
		//var_dump($dmObjInfo);		
      $this -> assertEquals( 0, self::$dbh -> getErrno() );		
      $this -> assertEquals( 2, count( $dmObjInfo['Groups'] ) );
      $this -> assertEquals( self::$groupNames[0], $dmObjInfo['Groups'][0]['GroupName'] );
      $this -> assertEquals( self::$groupNames[1], $dmObjInfo['Groups'][1]['GroupName'] );
   }
   
   function test_alterRemoveGroups01()
   {
      echo "\n---Begin to alter[removeGroups].\n"; 
      $options = array( 'Groups' => array( self::$groupNames[1] ) );
      self::$dbh -> alterRemoveGroups( self::$dmObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() ); 
		
      echo "   Begin to check results.\n";
      $dmObjInfo = self::$dbh -> listDM( self::$dmName );
		//var_dump($dmObjInfo);		
      $this -> assertEquals( 0, self::$dbh -> getErrno() );		
      $this -> assertEquals( 1, count( $dmObjInfo['Groups'] ) );
      $this -> assertEquals( self::$groupNames[0], $dmObjInfo['Groups'][0]['GroupName'] );
   }
   
   function test_alterRemoveGroups02()
   {
      echo "\n---Begin to alter[removeGroups, last group].\n"; 
      $options = array( 'Groups' => array( self::$groupNames[0] ) );
      self::$dbh -> alterRemoveGroups( self::$dmObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() ); 
		
      echo "   Begin to check results.\n";
      $dmObjInfo = self::$dbh -> listDM( self::$dmName );
		//var_dump($dmObjInfo);		
      $this -> assertEquals( 0, self::$dbh -> getErrno() );
      $this -> assertEquals( 0, count( $dmObjInfo['Groups'] ) );
   }
   
   function test_alterSetGroups()
   {
      echo "\n---Begin to alter[setGroups].\n"; 
      $options = array( 'Groups' => array( self::$groupNames[1] ) );
      self::$dbh -> alterSetGroups( self::$dmObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() ); 
		
      echo "   Begin to check results.\n";
      $dmObjInfo = self::$dbh -> listDM( self::$dmName );
		//var_dump($dmObjInfo);		
      $this -> assertEquals( 0, self::$dbh -> getErrno() );		
      $this -> assertEquals( 1, count( $dmObjInfo['Groups'] ) );
      $this -> assertEquals( self::$groupNames[1], $dmObjInfo['Groups'][0]['GroupName'] );
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to clean env.\n"; 
      echo "   Begin to drop domain in the end.\n"; 
      self::$dbh -> dropDM( self::$dmName, false );
		$errno = self::$dbh -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to dropDomain. Errno: ". $errno ."\n";
      }
   }
   
}
?>