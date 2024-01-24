/****************************************************
@description:      createCS / getCS / getName / listCS / dropCS, only cover required parameter
@testlink cases:   seqDB-7648 / seqDB-7650
@input:        1 test_createCS, cover required parameter[$name]
               2 test_getCS
               3 test_getName
               4 test_listCS, cover required parameter[null]
               5 test_dropCS
@output:     success
@modify list:
        2016-4-15 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class CSOperator02 extends BaseOperator 
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
   
   function createCS( $csName )
   {
      return $this -> commCreateCS( $csName );
   }
   
   function getCS( $csName )
   {
      return $cs = $this -> db -> getCS( $csName );
   } 
   
   function getName( $cs )
   {
      return $cs -> getName();
   } 
   
   function listCS( $csName )
   {
      $cursor = $this -> db -> listCS();
      while( $tmpInfo = $cursor -> next() )
      {
         if ( $tmpInfo['Name'] === $csName )
         {
            return $tmpInfo['Name'];
         }
      }
      return false;
   } 
   
   function dropCS( $csName, $ignoreNotExist )
   {
      $this -> commDropCS( $csName, $ignoreNotExist );
   }
   
}

class TestCS02 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new CSOperator02();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      
      echo "\n---Begin to drop cs in the begin.\n";
      self::$dbh -> dropCS( self::$csName, true );
   }
   
   function test_createCS()
   {
      echo "\n---Begin to create cs.\n";
      
      self::$dbh -> createCS( self::$csName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno ); 
   }
   
   function test_getCS()
   {
      echo "\n---Begin to getCS.\n";
      
      $name = self::$dbh -> getCS( self::$csName );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertNotEmpty( $name );
   }
   
   function test_getName()
   {
      echo "\n---Begin to getName.\n";
      $cs = self::$dbh -> getCS( self::$csName );
      $name = self::$dbh -> getName( $cs );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare name
      $this -> assertEquals( self::$csName, $name );
   }
   
   function test_listCS()
   {
      echo "\n---Begin to list cs.\n";
      
      $name = self::$dbh -> listCS( self::$csName );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare name
      $this -> assertEquals( self::$csName, $name );
   }
   
   function test_dropCS()
   {
      echo "\n---Begin to drop cs in the end.\n";
      
      self::$dbh -> dropCS( self::$csName, false );
      
      //compare exec results
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      //compare cs is not exist
      self::$dbh -> getCS( self::$csName );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -34, $errno );
   }
   
}
?>