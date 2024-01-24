/****************************************************
@description:      alter, setAttributes
@testlink cases:   seqDB-15155
@input:      setAttributes[name, Groups, AutoSplit]
@output:     fail: name
				 success: Groups, AutoSplit
@modify list:
        2018-4-26 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DomainOper15155 extends BaseOperator 
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
   
   function alterSetAttributes( $dmObj, $options )
   {		
      $dmObj -> setAttributes( $options );
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

class TestDomain15155 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $dmName;
   private static $dmObj;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
		echo "\n---Begin to run[seqDB-15155].\n"; 
      self::$dbh = new DomainOper15155();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "   Begin to ready parameter.\n";
         self::$dmName = self::$dbh -> COMMDOMAINNAME .'_15155';
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
   
   function test_alterSetAttributes01()
   {
      echo "\n---Begin to alter[setAttributes: Groups, AutoSplit].\n"; 
      $options = array( 'Groups' => array( self::$groupNames[1] ), 'AutoSplit' => true );
      self::$dbh -> alterSetAttributes( self::$dmObj, $options );
      $this -> assertEquals( 0, self::$dbh -> getErrno() ); 
		
      echo "   Begin to check results.\n"; 
      $dmInfo = self::$dbh -> listDM( self::$dmName );
		//var_dump($dmInfo);
      $this -> assertEquals( 0, self::$dbh -> getErrno() );     
      $this -> assertEquals( self::$dmName, $dmInfo['Name'] );
      $this -> assertTrue( $dmInfo['AutoSplit'] );
      $this -> assertEquals( 1, count( $dmInfo['Groups'] ) );
      $this -> assertEquals( self::$groupNames[1], $dmInfo['Groups'][0]['GroupName'] );
   }
   
   function test_alterSetAttributes02()
   {
      echo "\n---Begin to alter[setAttributes: Name].\n"; 
      $options = array( 'Test' => 'test15155' );
      self::$dbh -> alterSetAttributes( self::$dmObj, $options );
      $this -> assertEquals( -6, self::$dbh -> getErrno() ); 
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