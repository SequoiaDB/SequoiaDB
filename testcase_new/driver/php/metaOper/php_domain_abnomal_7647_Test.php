/****************************************************
@description:      domain, abnomal case
@testlink cases:   seqDB-7647
@input:        1 dropDomain, the domain is not exist.
@output:     $errno: -214
@modify list:
        2016-4-12 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DomainOper01 extends BaseOperator 
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
   
   function dropDM( $dmName, $ignoreNotExist )
   {
      $this -> commDropDomain( $dmName, $ignoreNotExist );
   }
   
}

class TestDomain01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $dmName;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DomainOper01();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "\n---Begin to ready parameter.\n";
         self::$dmName = self::$dbh -> COMMDOMAINNAME .'test';
         
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
   
   function test_dropDomain()
   {
      echo "\n---Begin to drop domain in the end.\n";
      
      self::$dbh -> dropDM( self::$dmName, false );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -214, $errno );
   }
   
}
?>