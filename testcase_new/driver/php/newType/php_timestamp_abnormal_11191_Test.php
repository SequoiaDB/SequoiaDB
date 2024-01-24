/****************************************************
@description:      Test SequoiaTimestamp, date range is invalid
@testlink cases:   seqDB-11191
@input:        crud, data type: date
@output:     -6
@modify list:
        2017-3-3 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DateTypeAb11191 extends BaseOperator 
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
      $recsArray = array(
         array( 'a' => 0, 'b' => new SequoiaTimestamp( "1901-12-31-23:59:59.999999" ) ),
         array( 'a' => 1, 'b' => new SequoiaTimestamp( "2038-01-01-00:00:00.000000" ) ),
         array( 'a' => 2, 'b' => new SequoiaTimestamp( "1901-12-31T23:59:59.999Z" ) ),
         array( 'a' => 3, 'b' => new SequoiaTimestamp( "2038-01-01T00:00:00.000Z" ) ),
         array( 'a' => 4, 'b' => new SequoiaTimestamp( "1901-12-31T23:59:59.999+0800" ) ),
         array( 'a' => 5, 'b' => new SequoiaTimestamp( "2038-01-01T00:00:00.000+0800" ) ),
         array( 'a' => 6, 'b' => new SequoiaTimestamp( "-2147483649" ) ),
         array( 'a' => 7, 'b' => new SequoiaTimestamp( "2147483648" ) )
      );
      
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
   }
   
   function findRecs( $clDB )
   {
      $findRecsArray = array() ;
      $cursor = $clDB -> find( '{b:{$type:1, $et:17}}', '{_id:{$include:0}}', '{a:1}' );   
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
      
      //var_dump( $findRecsArray );
      return $findRecsArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestDateAb11191 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DateTypeAb11191();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME;
      
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
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $recsArray = self::$dbh -> findRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 8, $recsArray );
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