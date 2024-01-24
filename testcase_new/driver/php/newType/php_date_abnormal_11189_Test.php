/****************************************************
@description:      Test SequoiaDate, date range is invalid
@testlink cases:   seqDB-11189
@input:        crud, data type: date
@output:     -6, and beyond the scop of validify does ot guarantee the correctness of the value
@modify list:
        2017-3-3 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DateTypeAb11189 extends BaseOperator 
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
         array( 'a' => 0, 'b' => new SequoiaDate( "1899-12-31" ) ),
         array( 'a' => 1, 'b' => new SequoiaDate( "10000-01-01" ) ),
         array( 'a' => 2, 'b' => new SequoiaDate( "0000-12-31T15:00:00.000Z" ) ),
         array( 'a' => 3, 'b' => new SequoiaDate( "10000-01-01T16:00:00.000Z" ) ),
         array( 'a' => 4, 'b' => new SequoiaDate( "0000-12-31T15:00:00.000+0800" ) ),
         array( 'a' => 5, 'b' => new SequoiaDate( "10000-01-01T16:00:00.000+0800" ) ),
         array( 'a' => 6, 'b' => new SequoiaDate( "-9223372036854775809" ) ),
         array( 'a' => 7, 'b' => new SequoiaDate( "9223372036854775808" ) )
      );
      
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $err = $clDB -> insert( $recsArray[$i] );
         
         if ( $i > 1 && $i < 6 ) {
            if ( $err['errno'] != -258 ) {
               throw new Exception("i = ".$i.", errno = ".$err['errno']);
            }
         } else {
            if ( $err['errno'] != 0 ) {
               throw new Exception("i = ".$i.", errno = ".$err['errno']);
            }            
         }
      }
   }
   
   function findRecs( $clDB )
   {
      $findRecsArray = array() ;
      $cursor = $clDB -> find( '{b:{$type:1, $et:9}}', '{_id:{$include:0}}', '{a:1}' );   
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

class TestDateAb11189 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DateTypeAb11189();
      
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
   }
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $actRecsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 4, $actRecsArray );
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to drop cl in the end.\n";
      
      self::$dbh -> dropCL( self::$csName, self::$clName, false );
      $errno = self::$dbh -> getErrno();
      if ($errno != 0) {
          throw new Exception("failed to drop cl, errno=".$errno);
      }
   }
  
}
?>