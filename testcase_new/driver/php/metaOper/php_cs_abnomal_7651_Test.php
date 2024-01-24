/****************************************************
@description:      createCS, the domain is not exist
@testlink cases:   seqDB-7648 / seqDB-7650
@input:        1 test_createCS, the domain is not exist
               2 test_getCS
               3 test_dropDomain
@output:     -214
@modify list:
        2016-4-15 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator01 extends BaseOperator 
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
   
   function createCS( $csName, $dmName )
   {
      $options = array( 'Domain' => $dmName );
      $this -> commCreateCS( $csName, $options );
   }
   
   function getCS( $csName )
   {
      return $cs = $this -> db -> getCS( $csName );
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

class TestCS01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $dmName;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CSOperator01();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "\n---Begin to ready parameter.\n";
         self::$csName = self::$dbh -> COMMCSNAME;
         self::$dmName = self::$dbh -> COMMDOMAINNAME;
         
         echo "\n---Begin to drop cs in the begin.\n";
         self::$dbh -> dropCS( self::$csName, true );
         
         echo "\n---Begin to drop domain in the begin.\n";
         self::$dbh -> dropDM( self::$dmName, true );
      }
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_createCS()
   {
      echo "\n---Begin to create cs, the domain is not exist.\n";
      
      self::$dbh -> createCS( self::$csName, self::$dmName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -214, $errno );
   }
   
   function test_getCS()
   {
      echo "\n---Begin to getCS.\n";
      
      $name = self::$dbh -> getCS( self::$csName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -34, $errno );
   }
   
}
?>