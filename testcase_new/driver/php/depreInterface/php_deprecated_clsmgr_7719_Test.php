/****************************************************
@description:      deprecated interface, base case
@testlink cases:   seqDB-7719/ seqDB-7720
@input:        1 selectGroup 
               2 getNodeName 
@output:     success
@modify list:
        2016-4-29 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DepreOperator03 extends BaseOperator 
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
   
   function selectGroup ( $groupNames )
   {
      return $this -> db -> selectGroup( $groupNames[0] );
   }
   
   function getMaster ( $rgDB )
   {
      return $rgDB -> getMaster();
   }
   
   function getNodeNum( $rgDB )
   {
      return $rgDB -> getNodeNum( 0 );
   }
   
   function getNodeName( $nodeDB )
   {
      return $nodeDB -> getNodeName();
   }
   
   /** the interface has bean deleted
   function getNodeStatus( $nodeDB )
   {
      return $nodeDB -> getStatus();
   }
   */
   
}

class TestDepre03 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $groupNames;
   private static $rgDB;
   private static $nodeDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DepreOperator03();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         
         self::$groupNames = self::$dbh -> commGetGroupNames();
      }
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_selectGroup()
   {
      echo "\n---Begin to selectGroup.\n";
      self::$rgDB = self::$dbh -> selectGroup( self::$groupNames );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_getMaster()
   {
      echo "\n---Begin to getMaster.\n";
      self::$nodeDB = self::$dbh -> getMaster( self::$rgDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_getNodeNum()
   {
      echo "\n---Begin to getNodeNum.\n";
      $num = self::$dbh -> getNodeNum( self::$rgDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      $this -> assertEquals( -1, $num );
   }
   
   function test_getNodeName()
   {
      echo "\n---Begin to getNodeName.\n";
      $name = self::$dbh -> getNodeName( self::$nodeDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      $this -> assertNotEmpty( $name );
   }
   
   /*
   function test_getNodeStatus()
   {
      echo "\n---Begin to getNodeStatus.\n";
      $status = self::$dbh -> getNodeStatus( self::$nodeDB );
      var_dump($status);
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      $this -> assertNotEmpty( $status );
   }
   */
}
?>