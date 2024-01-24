/****************************************************
@description:      createDomain / alter / listDomain, cover all param, and type is string
@testlink cases:   seqDB-7645 / seqDB-7646
@input:      1 test_createDomain, $options: string
             2 test_listDomains, cover all param, $condition/$selector/$orderBy: string
                     $hint: null   -----[from doc]This parameter is reserved and must be null
             3 test_alter, $options: string
             4 test_listDomains, again
             5 test_dropDomain
@output:     success
@modify list:
        2016-4-12 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DomainOper07 extends BaseOperator 
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
      $options = json_encode( array( 'Groups' => array( $groupNames[0] ), 'AutoSplit' => true ) );
      $this -> commCreateDomain( $dmName, $options );
   }
   
   function alterDM( $dmName, $groupNames )
   {
      $options = json_encode( array( 'Groups' => array( $groupNames[1] ), 'AutoSplit' => false ) );
      $dm = $this -> db -> getDomain( $dmName );
      
      $dm -> alter( $options );
   }
   
   function listDM( $dmName1, $dmName2 )
   {
      $condition = '{"\$or": [{"Name": "'. $dmName1 .'"},{"Name": "'. $dmName2.'"}]}';
      $selector  = '{"_id": {"\$include": 0} }';
      $orderby   = json_encode( array( 'Name' => -1 ) );
      $hint = null; 
      
      $cursor = $this -> db -> listDomain( $condition, $selector, $orderby, $hint );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to list domain. Errno: ". $errno ."\n";
      }
      
      $tmpArray = array();
      while( $tmpInfo = $cursor -> next() )
      {
         array_push( $tmpArray, $tmpInfo );
      }
      
      return $tmpArray;
   } 
   
   function dropDM( $dmName, $ignoreNotExist )
   {
      $this -> commDropDomain( $dmName, $ignoreNotExist );
   }
   
}

class TestDomain07 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $dmName1;
   private static $dmName2;
   private static $dmName3;
   private static $groupNames;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DomainOper07();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "\n---Begin to ready parameter.\n";
         self::$dmName1 = self::$dbh -> COMMDOMAINNAME .'_1';
         self::$dmName2 = self::$dbh -> COMMDOMAINNAME .'_2';
         self::$dmName3 = self::$dbh -> COMMDOMAINNAME .'_3';
         self::$groupNames = self::$dbh -> commGetGroupNames();
         
         echo "\n---Begin to drop domain[". self::$dmName1 .",". self::$dmName2 .",". self::$dmName3 ."] in the begin.\n";
         self::$dbh -> dropDM( self::$dmName1, true );
         self::$dbh -> dropDM( self::$dmName2, true );
         self::$dbh -> dropDM( self::$dmName3, true );
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
      echo "\n---Begin to create domain[". self::$dmName1 .",". self::$dmName2 .",". self::$dmName3 ."].\n";
      
      self::$dbh -> createDM( self::$dmName1, self::$groupNames );
      $errno1 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno1 ); 
      
      self::$dbh -> createDM( self::$dmName2, self::$groupNames );
      $errno2 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno2 ); 
      
      self::$dbh -> createDM( self::$dmName3, self::$groupNames );
      $errno3 = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno3 ); 
   }
   
   function test_listDomainBeforeAlterOper()
   {
      echo "\n---Begin to list domain before alterOper.\n";
      
      $tmpArray = self::$dbh -> listDM( self::$dmName1, self::$dmName2 );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare result for $condition and $orderby
      $recsNum = count( $tmpArray );
      $this -> assertEquals( 2, $recsNum );
      
      $name2  = $tmpArray[0]['Name'];
      $this -> assertEquals( self::$dmName2, $name2 );
      
      $name1 = $tmpArray[1]['Name'];
      $this -> assertEquals( self::$dmName1, $name1 );

      //compare result for $selector
      $this -> assertArrayNotHasKey( "_id",$tmpArray[1] );
      
      //compare AutoSplit
      $split = $tmpArray[1]['AutoSplit'];
      $this -> assertTrue( $split );
      
      //compare Groups
      $rgName = $tmpArray[1]['Groups'][0]['GroupName'];
      $this -> assertEquals( self::$groupNames[0], $rgName );
                  
      $rgNum = count( $tmpArray[1]['Groups'] );
      $this -> assertEquals( 1, $rgNum );
   }
   
   function test_alter()
   {
      echo "\n---Begin to alter domain.\n";
      
      self::$dbh -> alterDM( self::$dmName1, self::$groupNames );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_listDomainAfterAlter()
   {
      echo "\n---Begin to list domain after alterOper.\n";
      
      $tmpArray = self::$dbh -> listDM( self::$dmName1, self::$dmName2 );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare AutoSplit
      $split = $tmpArray[1]['AutoSplit'];
      $this -> assertEquals( false, $split );
      
      //compare Groups
      $rgName = $tmpArray[1]['Groups'][0]['GroupName'];
      $this -> assertEquals( self::$groupNames[1], $rgName );
                  
      $rgNum = count( $tmpArray[1]['Groups'] );
      $this -> assertEquals( 1, $rgNum );
   }
   
   function test_dropDomain()
   {
      echo "\n---Begin to drop domain[". self::$dmName1 .",". self::$dmName2 ."] in the end.\n";
      
      //drop domain1, and compare exec results
      self::$dbh -> dropDM( self::$dmName1, false );
      self::$dbh -> dropDM( self::$dmName2, false );
      self::$dbh -> dropDM( self::$dmName3, false );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>