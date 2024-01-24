/****************************************************
@description:      createIndex / getIndex / explain / dropIndex, base case
@testlink cases:   seqDB-7669
@input:        1 createCL
               2 createIndex, cover required parameter[$indexDef, $indexName]
               3 getIndex, cover required parameter[]
               4 dropIndex
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class IndexOper05 extends BaseOperator 
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
   
   function createCL( $csName, $clName )
   {
      $options = null;
      return $this -> commCreateCL( $csName, $clName, $options, true );
   }
   
   function createIndex( $clDB, $idxName )
   {
      $indexDef = array( 'a' => 1 );
      $clDB -> createIndex( $indexDef, $idxName );
   }
   
   function getIndex( $clDB, $idxName )
   {
      $cursor = $clDB -> getIndex();
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to get index. Errno: ". $errno ."\n";
      }
      
      while( $idxInfo = $cursor -> next() )
      {
         if( $idxInfo['IndexDef']['name'] === $idxName )
         {
            return $idxInfo;
         }
      }
   }
   
   function insertRecs( $clDB )
   {
      $recs = array( 'a' => 1, 'b' => 1 );
      $clDB -> insert( $recs );
   }
   
   function explain( $clDB )
   {
      $options   = array( 'run' => true );
      $cursor = $clDB -> explain( $options );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to exec explain. Errno: ". $errno ."\n";
      }
      
      $tmpArray = array() ;
      while( $record = $cursor -> next() )
      {
         array_push( $tmpArray, $record );
      }
      
      return $tmpArray;
   }   
   
   function findRecs( $clDB )
   {
      $hint      = array( '' => 'a' );  
      $cursor = $clDB -> find( null, null, null, $hint );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to find records. Errno: ". $errno ."\n";
      }
      
      $findRecsArray = array() ;
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
      return $findRecsArray;
   }
   
   function dropIndex( $clDB, $idxName )
   {
      $clDB -> dropIndex( $idxName );
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestIndex05 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $idxName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new IndexOper05();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName  = self::$dbh -> COMMCLNAME;
      self::$idxName = self::$dbh -> CHANGEDPREFIX .'_index';
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_createIndex()
   {
      echo "\n---Begin to create index.\n";
      
      self::$dbh -> createIndex( self::$clDB, self::$idxName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
      
   function test_getIndexAfterCreateIdx()
   {
      echo "\n---Begin to get index after create index.\n";
      
      $idxInfo = self::$dbh -> getIndex( self::$clDB, self::$idxName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $actName = self::$idxName;
      $actKey  = array( 'a' => 1);
      $actUnique    = false;
      $actEnforced  = false;
      $actIndexFlag = "Normal";
      $expName = $idxInfo['IndexDef']['name'];
      $expKey  = $idxInfo['IndexDef']['key'];
      $expUnique     = $idxInfo['IndexDef']['unique'];
      $expEnforced   = $idxInfo['IndexDef']['enforced'];
      $expIndexFlag  = $idxInfo['IndexFlag'];
      $this -> assertEquals( $actName, $expName );
      $this -> assertEquals( $actKey,  $expKey );
      $this -> assertEquals( $actUnique, $expUnique );
      $this -> assertEquals( $actEnforced,  $expEnforced );
      $this -> assertEquals( $actIndexFlag, $expIndexFlag );
   }
   
   function test_explain()
   {
      echo "\n---Begin to exec explain.\n";
      
      $explainInfo = self::$dbh -> explain( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $actName      = self::$csName .'.'. self::$clName;
      $actIndexName = '';
      $actScanType  = 'tbscan';
      $actRun       = true;
      $actIXBound   = null;
      $expName      = $explainInfo[0]['Name'];
      $expIndexName = $explainInfo[0]['IndexName'];
      $expScanType  = $explainInfo[0]['ScanType'];
      $expRun       = $explainInfo[0]['Query']['$and'][0]['run']['$et'];
      $expIXBound   = $explainInfo[0]['IXBound'];
      $this -> assertEquals( $actName, $expName );
      $this -> assertEquals( $actIndexName, $expIndexName );
      $this -> assertEquals( $actScanType, $expScanType );
      $this -> assertEquals( $actRun, $expRun );
      $this -> assertEquals( $actIXBound, $expIXBound );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $actCount = 1;
      $actA  = 1;
      $actB  = 1;
      $this -> assertCount( 1, $recsArray );
      $this -> assertEquals( 1, $actA );
      $this -> assertEquals( 1, $actB );
   }
   
   function test_dropIndex()
   {
      echo "\n---Begin to drop index.\n";
      
      self::$dbh -> dropIndex( self::$clDB, self::$idxName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_getIndexAfterDropIdx()
   {
      echo "\n---Begin to get index after drop index.\n";
      
      self::$dbh -> getIndex( self::$clDB, self::$idxName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
   }
 
   function test_dropCL()
   {
      echo "\n---Begin to drop cl in the end.\n";
      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>