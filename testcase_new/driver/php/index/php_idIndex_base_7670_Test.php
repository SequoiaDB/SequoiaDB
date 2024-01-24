/****************************************************
@description:      createIdIndex / getIndex / dropIdIndex, base case
@testlink cases:   seqDB-7669
@input:        1 createCL
               2 createIndex:
                 cover required parameter[]
               3 getIndex
               4 dropIndex
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class IndexOper01 extends BaseOperator 
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
   
   function createIdIndex( $clDB )
   {
      $clDB -> createIdIndex();
   }
   
   function getIndex( $clDB )
   {
      $cursor = $clDB -> getIndex( '$id' );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to get index. Errno: ". $errno ."\n";
      }
      
      while( $idxInfo = $cursor -> next() )
      {
         if( $idxInfo['IndexDef']['name'] === '$id' )
         {
            return $idxInfo;
         }
      }
   }
   
   function dropIdIndex( $clDB )
   {
      $clDB -> dropIdIndex();
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestIndex01 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $idxName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new IndexOper01();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName  = self::$dbh -> COMMCLNAME;
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
      
      echo "\n---Begin to drop id index in the begin.\n";
      self::$dbh -> dropIdIndex( self::$clDB );
   }
   
   function test_createIdIndex()
   {
      echo "\n---Begin to create id index.\n";
      
      self::$dbh -> createIdIndex( self::$clDB ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_getIndexAfterCreateIdx()
   {
      echo "\n---Begin to get id index after create index.\n";
      
      $idxInfo = self::$dbh -> getIndex( self::$clDB ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $actName = '$id';
      $actKey  = array( '_id' => 1);
      $actUnique    = true;
      $actEnforced  = true;
      $actIndexFlag = 'Normal';
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
   
   function test_dropIdIndex()
   {
      echo "\n---Begin to drop index.\n";
      
      self::$dbh -> dropIdIndex( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_getIndexAfterDropIdx()
   {
      echo "\n---Begin to get index after drop index.\n";
      
      self::$dbh -> getIndex( self::$clDB ); 
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