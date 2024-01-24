/****************************************************
@description:      createIndex, abnomal case
@testlink cases:   seqDB-7672
@input:        1 createCL
               2 insert records, exists repeat records
               2 createIndex, unique: true
@output:     -38
@modify list:
        2016-4-25 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class IndexOper04 extends BaseOperator 
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
   
   function insertRecs( $clDB )
   {
      $recs = array( 'a' => 1 );
      
      $clDB -> insert( $recs );
      $clDB -> insert( $recs );
   }
   
   function createIndex( $clDB, $idxName )
   {
      $indexDef = array( 'a' => 1 );
      $clDB -> createIndex( $indexDef, $idxName, true );
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestIndex04 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $idxName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new IndexOper04();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName  = self::$dbh -> COMMCLNAME;
      self::$idxName = self::$dbh -> CHANGEDPREFIX .'_index';
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_createIndex()
   {
      echo "\n---Begin to create index.\n";
      
      self::$dbh -> createIndex( self::$clDB, self::$idxName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -38, $errno );
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