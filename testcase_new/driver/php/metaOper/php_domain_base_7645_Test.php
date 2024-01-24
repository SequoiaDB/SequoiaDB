/****************************************************
@description:      createDomain / getDomain / listDomain / dropDomain, only cover required parameter
@testlink cases:   seqDB-7645 / seqDB-7646
@input:        1 test_createDomain, cover required parameter[$name]
               2 test_getDomain
               3 test_listDomains, cover required parameter[null]
               4 test_dropDomain
@output:     success
@modify list:
        2016-4-12 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DomainOper02 extends BaseOperator 
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
   
   function createDM( $dmName )
   {
      $this -> commCreateDomain( $dmName );
   }
   
   function getDM( $dmName )
   {
      return $this -> db -> getDomain( $dmName );
   } 
   
   function listDM( $dmName )
   {
      $cursor = $this -> db -> listDomain();
      while( $tmpInfo = $cursor -> next() )
      {
         if ( $tmpInfo['Name'] === $dmName )
         {
            return $tmpInfo['Name'];
         }
      }
      return false;
   } 
   
   function dropDM( $dmName, $ignoreNotExist )
   {
      $this -> commDropDomain( $dmName, $ignoreNotExist );
   }
   
}

class TestDomain02 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $dmName;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DomainOper02();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "\n---Begin to ready parameter.\n";
         self::$dmName = self::$dbh -> COMMDOMAINNAME;
         
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
   
   function test_createDomain()
   {
      echo "\n---Begin to create domain.\n";
      
      self::$dbh -> createDM( self::$dmName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_getDomain()
   {
      echo "\n---Begin to get domain.\n";
      
      $name = self::$dbh -> getDM( self::$dmName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertNotEmpty( $name ); //expect results: is not empty
   }
   
   function test_listDomain()
   {
      echo "\n---Begin to list domain.\n";
      
      $name = self::$dbh -> listDM( self::$dmName );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare name
      $this -> assertEquals( self::$dmName, $name );
   }
   
   function test_dropDomain()
   {
      echo "\n---Begin to drop domain in the end.\n";
      
      self::$dbh -> dropDM( self::$dmName, false );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare domain is not exist
      self::$dbh -> getDM( self::$dmName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -214, $errno );
   }
   
}
?>