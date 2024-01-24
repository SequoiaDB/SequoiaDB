/****************************************************
@description:      listCS / listCL / listGroups, cover all param, and type is array
@testlink cases:   seqDB-7645 / seqDB-7646
@input:      1 test_createDomain, $options: multi groups
             2 test_createCL
             3 test_createCS
             4 test_listCS, cover all param, $condition/$selector/$orderBy/$hint: null
                     -------[from doc]This parameter is reserved and must be null
             5 test_listCL, cover all param, $condition/$selector/$orderBy/$hint: null
                     -------[from doc]This parameter is reserved and must be null
             6 test_listGroups, cover all param, $condition/$selector/$orderBy/$hint: null
                     -------[from doc]This parameter is reserved and must be null
             7 test_dropDomain
@output:     success
@modify list:
        2016-4-18 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DomainOper04 extends BaseOperator 
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
   
   function createDM( $dmName, $groupNames )
   {
      $options = array( 'Groups' => $groupNames, 'AutoSplit' => true );
      $this -> commCreateDomain( $dmName, $options );
   }
   
   function createCS( $csName, $dmName )
   {
      $options = array( 'Domain' => $dmName );
      $this -> commCreateCS( $csName, $options );
   }
   
   function createCL( $csName, $clName )
   {
      $this -> commCreateCL( $csName, $clName, null, true );
   }
   
   function getDM( $dmName )
   {
      return $this -> db -> getDomain( $dmName );
   } 
   
   function listCS( $dmDB )
   {
      $condition = null;
      $selector  = null;
      $orderby   = null;
      $hint = null;
      $cursor = $dmDB -> listCS( $condition, $selector, $orderby, $hint );
      
      while( $csInfo = $cursor -> next() )
      {
         return $csInfo;
      }
      return false;
   }
   
   function listCL( $dmDB )
   {
      $condition = null;
      $selector  = null;
      $orderby   = null;
      $hint = null;
      
      $cursor = $dmDB -> listCL( $condition, $selector, $orderby, $hint );
      
      while( $clInfo = $cursor -> next() )
      {
         return $clInfo;
      }
      return false;
   }
   
   function listGroup( $dmDB )
   {
      $condition = null;
      $selector  = null;
      $orderby   = null;
      $hint = null;
      
      $cursor = $dmDB -> listGroup( $condition, $selector, $orderby, $hint );
      
      while( $rgInfo = $cursor -> next() )
      {
         return $rgInfo;
      }
      return false;
   }
   
   function dropDM( $dmName, $ignoreNotExist )
   {
      $this -> commDropDomain( $dmName, $ignoreNotExist );
   }
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
}

class TestDomain04 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $dmName;
   private static $csName;
   private static $clName;
   private static $groupNames;
   private static $dmDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DomainOper04();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "\n---Begin to ready parameter.\n";
         self::$dmName = self::$dbh -> COMMDOMAINNAME;
         self::$csName = self::$dbh -> COMMCSNAME;
         self::$clName = self::$dbh -> COMMCLNAME;
         self::$groupNames = self::$dbh -> commGetGroupNames();
         
         echo "\n---Begin to drop domain cs/domain in the begin.\n";
         self::$dbh -> dropCS( self::$csName, true );
         self::$dbh -> dropDM( self::$dmName, true );
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
         echo "\n---Begin to judge groupNumber.\n";
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_createDomain()
   {
      echo "\n---Begin to create domain[". self::$dmName ."].\n";
      
      self::$dbh -> createDM( self::$dmName,  self::$groupNames );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_createCS()
   {
      echo "\n---Begin to create cs[". self::$csName ."].\n";
      
      self::$dbh -> createCS( self::$csName, self::$dmName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_createCL()
   {
      echo "\n---Begin to create cl[". self::$csName .".". self::$clName ."].\n";
      
      self::$dbh -> createCL( self::$csName, self::$clName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_getDomain()
   {
      echo "\n---Begin to get domain.\n";
      
      self::$dmDB = self::$dbh -> getDM( self::$dmName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_listCS()
   {
      echo "\n---Begin to listCS.\n";
      
      $cs = self::$dbh -> listCS( self::$dmDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      $this -> assertEquals( self::$csName, $cs["Name"] ); 
   }
   
   function test_listCL()
   {
      echo "\n---Begin to listCL.\n";
      
      $cl = self::$dbh -> listCL( self::$dmDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      $actualName = self::$csName. '.' .self::$clName;
      $this -> assertEquals( $actualName, $cl["Name"] ); 
   }
   
   function test_listGroup()
   {
      echo "\n---Begin to listGroup.\n";
      
      $rg = self::$dbh -> listGroup( self::$dmDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
      
      $rgName0 = $rg["Groups"][0]["GroupName"];
      $rgName1 = $rg["Groups"][1]["GroupName"];
      $rgName2 = $rg["Groups"][2]["GroupName"];
      $this -> assertEquals( self::$groupNames[0], $rgName0 ); 
      $this -> assertEquals( self::$groupNames[1], $rgName1 ); 
      $this -> assertEquals( self::$groupNames[2], $rgName2 ); 
   }
   
   function test_dropCS()
   {
      echo "\n---Begin to drop cs[". self::$csName ."] in the end.\n";
      
      self::$dbh -> dropCS( self::$csName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_dropDomain()
   {
      echo "\n---Begin to drop domain[". self::$dmName ."] in the end.\n";
      
      self::$dbh -> dropDM( self::$dmName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>