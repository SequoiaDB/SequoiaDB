/****************************************************
@description:      createIndex / getIndex / explain, cover all parameter
@testlink cases:   seqDB-7669 / seqDB-7671
@input:        1 createCL
               2 createIndex, cover all parameter, $indexDef: array
               3 getIndex, $indexName
               4 dropIndex
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class IndexOper07 extends BaseOperator 
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
      $indexDef = json_encode( array( 'a' => 1 ) );
      $clDB -> createIndex( $indexDef, $idxName, true, true, 128 );
   }
   
   function getIndex( $clDB, $idxName )
   {
      $cursor = $clDB -> getIndex( $idxName );
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
   
   function explain( $clDB, $idxName )
   {
      $options   = json_encode( array( 'run' => true ) );
      $condition = json_encode( array( 'a' => '1' ) );
      $selector  = json_encode( array( 'a' => '', 'b' => 'hello' ) );
      $orderby   = json_encode( array( 'a' => 1 ) );
      $hint      = json_encode( array( ''  => $idxName ) );
      $numToSkip = 1;
      $numToReturn = 15;
      $flag = 0;
      
      $cursor = $clDB -> explain( $condition, $selector, $orderby, $hint, 
                                  $numToSkip, $numToReturn, $flag, $options );
                                  
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to find records. Errno: ". $errno ."\n";
      }
      
      $tmpArray = array() ;
      while( $record = $cursor -> next() )
      {
         array_push( $tmpArray, $record );
      }
      
      return $tmpArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestIndex07 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $idxName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new IndexOper07();
      
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
   
   function test_getIndex()
   {
      echo "\n---Begin to get index.\n";
      
      $idxInfo = self::$dbh -> getIndex( self::$clDB, self::$idxName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $actName = self::$idxName;
      $actKey  = array( 'a' => 1);
      $actUnique    = true;
      $actEnforced  = true;
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
      
      $explainInfo = self::$dbh -> explain( self::$clDB, self::$idxName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $actName       = self::$csName .'.'. self::$clName;
      $actIndexName  = self::$idxName;
      $actScanType   = 'ixscan';
      $actRun        = '1';
      $actStartBound = '1'; 
      $actEndBound   = '1'; 
      $expName       = $explainInfo[0]['Name'];
      $expIndexName  = $explainInfo[0]['IndexName'];
      $expScanType   = $explainInfo[0]['ScanType'];
      $expRun        = $explainInfo[0]['Query']['$and'][0]['a']['$et'];
      $expStartBound = $explainInfo[0]['IXBound']['a'][0][0];
      $expEndBound   = $explainInfo[0]['IXBound']['a'][0][1];
      $this -> assertEquals( $actName, $expName );
      $this -> assertEquals( $actIndexName, $expIndexName );
      $this -> assertEquals( $actScanType, $expScanType );
      $this -> assertEquals( $actRun, $expRun );
      $this -> assertEquals( $actStartBound, $expStartBound );
      $this -> assertEquals( $actEndBound, $expEndBound );
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