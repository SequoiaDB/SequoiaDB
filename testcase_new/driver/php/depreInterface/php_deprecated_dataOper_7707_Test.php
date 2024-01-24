/****************************************************
@description:      deprecated interface, base case
@testlink cases:   seqDB-7707
@input:        1 listDomains 
               2 listCSs / selectCS / dropCollectionSpace
               3 selectCollection / getCollectionName / listCollections / dropCollection
               4 deleteIndex
@output:     success
@modify list:
        2016-4-27 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DepreOperator02 extends BaseOperator 
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
   
   function createDomain( $dmName )
   {
      $this -> commCreateDomain( $dmName );
   }  
   
   function listDomains( $dmName )
   {
      $cursor = $this -> db -> listDomains();
      while( $tmpInfo = $cursor -> next() )
      {
         if ( $tmpInfo['Name'] === $dmName )
         {
            return $tmpInfo['Name'];
         }
      }
      return false;
   } 
   
   function selectCS( $csName )
   { 
      $pageSize = 8192;
      return $this -> db -> selectCS( $csName, $pageSize );
   }
   
   function listCSs( $csName )
   {
      $cursor = $this -> db -> listCSs();
      while( $tmpInfo = $cursor -> next() )
      {
         if ( $tmpInfo['Name'] === $csName )
         {
            return $tmpInfo['Name'];
         }
      }
      return false;
   } 
   
   function selectCollection( $csDB, $clName )
   {
      return $csDB -> selectCollection( $clName, null );
   }
   
   function getCollectionName( $clDB )
   {
      return $clDB -> getCollectionName(); 
   }
   
   function listCollections( $csName, $clName )
   {
      $cursor = $this -> db -> listCollections();
      $cl = $csName .'.'. $clName;
      while( $tmpInfo = $cursor -> next() )
      {
         if( $tmpInfo = $cl )
         {
            return $tmpInfo;
         }
      }
   }
   
   function createIndex( $clDB, $idxName )
   {
      $indexDef = array( 'a' => 1 );
      $clDB -> createIndex( $indexDef, $idxName, true, true, 128 );
   }
   
   function deleteIndex( $clDB, $idxName )
   {
      $clDB -> deleteIndex( $idxName );
   }
   
   function dropCollectionSpace( $csName )
   {
      $this -> db -> dropCollectionSpace( $csName );
   }
   
   function dropCollection( $csDB, $csName )
   {
      $csDB -> dropCollection( $csName );
   }
   
}

class TestDepre02 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $dmName;
   private static $csName;
   private static $clName;
   private static $idxName;
   private static $csDB;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DepreOperator02();
      
      echo "\n---Begin to ready parameter.\n";
      self::$dmName = self::$dbh -> COMMDOMAINNAME;
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME;
      self::$idxName = self::$dbh -> CHANGEDPREFIX .'_index';
      
      echo "\n---Begin to drop cs in the begin.\n";
      self::$dbh -> dropCollectionSpace( self::$csName );
      
      echo "\n---Begin to create cs in the begin.\n";
      self::$csDB = self::$dbh -> selectCS( self::$csName );
      
      echo "\n---Begin to create domain in the begin.\n";
      if( self::$dbh -> commIsStandlone() === false )
      {
         self::$dbh -> createDomain( self::$dmName );
      }
   }
   
   function test_listDomains()
   {
      echo "\n---Begin to listDomains.\n";
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         $name = self::$dbh -> listDomains( self::$dmName );
         
         //compare exec results
         $errno = self::$dbh -> getErrno();
         $this -> assertEquals( 0, $errno );
         
         //compare name
         $this -> assertEquals( self::$dmName, $name );
      }
   }
   
   function test_listCSs()
   {
      echo "\n---Begin to listCSs.\n";
      
      $name = self::$dbh -> listCSs( self::$csName );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare name
      $this -> assertEquals( self::$csName, $name );
   }
   
   function test_selectCollection()
   {
      echo "\n---Begin to selectCollection.\n";
      
      self::$clDB = self::$dbh -> selectCollection( self::$csDB, self::$clName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_getCollectionName()
   {
      echo "\n---Begin to getCollectionName.\n";
      
      $name = self::$dbh -> getCollectionName( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertEquals( self::$clName, $name );
   }
   
   function test_listCollections()
   {
      echo "\n---Begin to listCollections.\n";
      
      $name = self::$dbh -> listCollections( self::$csName, self::$clName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $expRecs = self::$csName .'.'. self::$clName;
      $this -> assertEquals( $expRecs, $name );
   }
   
   function test_createIndex()
   {
      echo "\n---Begin to create index.\n";
      
      self::$dbh -> createIndex( self::$clDB, self::$idxName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_deleteIndex()
   {
      echo "\n---Begin to deleteIndex.\n";
      
      self::$dbh -> deleteIndex( self::$clDB, self::$idxName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_dropCollection()
   {
      echo "\n---Begin to dropCollection.\n";
      
      self::$dbh -> dropCollection( self::$csDB, self::$clName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_dropCollectionSpace()
   {
      echo "\n---Begin to dropCollectionSpace.\n";
      
      self::$dbh -> dropCollectionSpace( self::$csName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
}
?>