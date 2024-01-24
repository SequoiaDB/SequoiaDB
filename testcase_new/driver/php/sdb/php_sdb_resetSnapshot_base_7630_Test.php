/****************************************************
@description:      resetSnapshot, base case
@testlink cases:   seqDB-7630
@input:      1 resetSnapshot, 
               1) cover only required param[]
               2) cover all param[$condition: array]
               2) cover all param[$condition: string]
@output:     success
@modify list:
        2016-4-15 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SdbResetSnapshot extends BaseOperator 
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
   
   function resetSnapshot( $paramNum )
   {
      if( $paramNum === null )
      {  
         $cursor = $this -> db -> resetSnapshot();
      }
      else if( $paramNum === 'array' )
      {
         $condition = array( 'Type' => 'sessions', 'SessionID' => 1 );
         $cursor = $this -> db -> resetSnapshot( $condition );
      }
      else if( $paramNum === 'string' )
      {
         $condition = '{Type: "sessions", SessionID: 1}';
         $cursor = $this -> db -> resetSnapshot( $condition );
      }
   }
   /*
   function checkResultBySnapshot()
   {
      $cursor = $this -> db -> snapshot( SDB_SNAP_SESSIONS_CURRENT );
   }*/
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
}

class TestSdbResetSnapshot extends PHPUnit_Framework_TestCase
{
   
   protected static $dbh;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SdbResetSnapshot();
   }
   
   function test_resetSnapshotByParamMust()
   {
      echo "\n---Begin to exec resetSnapshot[by required parameter].\n";
      
      self::$dbh -> resetSnapshot( null );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_resetSnapshotParamArray()
   {
      echo "\n---Begin to exec resetSnapshot[by all parameter, type: Array].\n";
      
      self::$dbh -> resetSnapshot( 'array' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_resetSnapshotByParamString()
   {
      echo "\n---Begin to exec resetSnapshot[by all parameter, type: String].\n";
      
      self::$dbh -> resetSnapshot( 'string' );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>