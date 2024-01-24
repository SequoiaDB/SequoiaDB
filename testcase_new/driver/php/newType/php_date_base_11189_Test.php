/****************************************************
@description:      Test SequoiaDate, date range is valid
@testlink cases:   seqDB-11189
@input:        crud, data type: date
@output:     success
@modify list:
        2017-11-30 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class DateType11189 extends BaseOperator 
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
   
   function insertRecs( $clDB, $localDate )
   {
      $recsArray = array( 
         array( 'a' => 0,  'b' => $localDate ), 
         array( 'a' => 1,  'b' => new SequoiaDate( "1901-01-01" ) ), 
         array( 'a' => 2,  'b' => new SequoiaDate( "9999-12-31" ) ), 
         array( 'a' => 3,  'b' => new SequoiaDate( "0001-01-01T00:00:00.000Z" ) ), 
         array( 'a' => 4,  'b' => new SequoiaDate( "1899-12-31T00:00:00.000Z" ) ), 
         array( 'a' => 5,  'b' => new SequoiaDate( "1900-01-01T00:00:00.000Z" ) ), 
         array( 'a' => 6,  'b' => new SequoiaDate( "9999-12-31T15:50:50.000Z" ) ), 
         array( 'a' => 7,  'b' => new SequoiaDate( "0001-01-01T00:00:00.000+0800" ) ), 
         array( 'a' => 8,  'b' => new SequoiaDate( "1899-12-31T00:00:00.000+0800" ) ), 
         array( 'a' => 9,  'b' => new SequoiaDate( "1900-01-01T00:00:00.000+0800" ) ), 
         array( 'a' => 10, 'b' => new SequoiaDate( "9999-12-31T23:50:50.000+0800" ) ), 
         array( 'a' => 11, 'b' => new SequoiaDate( "-2147483648" ) ), 
         array( 'a' => 12, 'b' => new SequoiaDate( "2147483648" ) ),
         array( 'a' => 13, 'b' => new SequoiaDate( "-9223372036854775808" ) ), 
         array( 'a' => 14, 'b' => new SequoiaDate(  "9223372036854775807" ) ), 
         array( 'a' => 15, 'b' => new SequoiaDate( "2209017600" ) ), 
         array( 'a' => 16, 'b' => new SequoiaDate( "253402272000" ) ), 
         array( 'a' => 17, 'b' => new SequoiaDate( "2209017601" ) ), 
         array( 'a' => 18, 'b' => new SequoiaDate( "253402272001" ) ),
         array( 'a' => 19, 'b' => new SequoiaDate( -9223372036854775807 ) ), // php has no 64 bits, the largest int 32 bits
         array( 'a' => 20, 'b' => new SequoiaDate(  9223372036854775807 ) ), 
         array( 'a' => 21, 'b' => new SequoiaDate( -2147483648 ) ), 
         array( 'a' => 22, 'b' => new SequoiaDate(  2147483647 ) ), 
         array( 'a' => 23, 'b' => new SequoiaDate( -9223372036854775809 ) ),    // avalid value
         array( 'a' => 24, 'b' => new SequoiaDate(  9223372036854775808 ) ),    // avalid value
         array( 'a' => 25, 'b' => new SequoiaDate( "-9223372036854775809" ) ),  // avalid value
         array( 'a' => 26, 'b' => new SequoiaDate(  "9223372036854775808" ) )   // avalid value
      );
      
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
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

class TestDate11189 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   private static $localDate;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DateType11189();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME;
      self::$localDate = new SequoiaDate();
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB, self::$localDate );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $actRecsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 27, $actRecsArray );
      
      $expRecsArray = array( 
         array( 'a' => 0,  'b' => self::$localDate ), 
         array( 'a' => 1,  'b' => "1901-01-01" ), 
         array( 'a' => 2,  'b' => "9999-12-31" ), 
         array( 'a' => 3,  'b' => "0001-01-01" ), 
         array( 'a' => 4,  'b' => "1899-12-31" ), 
         array( 'a' => 5,  'b' => "1900-01-01" ), 
         array( 'a' => 6,  'b' => "9999-12-31" ), 
         array( 'a' => 7,  'b' => "0001-01-01" ), 
         array( 'a' => 8,  'b' => "1899-12-31" ), 
         array( 'a' => 9,  'b' => "1900-01-01" ), 
         array( 'a' => 10, 'b' => "9999-12-31" ), 
         array( 'a' => 11, 'b' => "1969-12-07" ), 
         array( 'a' => 12, 'b' => "1970-01-26" ),
         array( 'a' => 13, 'b' => "-9223372036854775808" ), 
         array( 'a' => 14, 'b' => "9223372036854775807" ), 
         array( 'a' => 15, 'b' => "1970-01-26" ), 
         array( 'a' => 16, 'b' => "1978-01-12" ), 
         array( 'a' => 17, 'b' => "1970-01-26" ), 
         array( 'a' => 18, 'b' => "1978-01-12" ),
         array( 'a' => 19, 'b' => "-9223372036854775807" ),  
         array( 'a' => 20, 'b' =>  "9223372036854775807" ), 
         array( 'a' => 21, 'b' => "1969-12-07" ),  // -2147483648
         array( 'a' => 22, 'b' => "1970-01-26" ),  //  2147483647
         array( 'a' => 23, 'b' => "1970-01-01" ),  // -9223372036854775809
         array( 'a' => 24, 'b' => "1970-01-01" ),  //  9223372036854775808
         array( 'a' => 25, 'b' =>  "9223372036854775807" ), // avalid
         array( 'a' => 26, 'b' => "-9223372036854775808" )  // avalid
      );
      
      for ($i = 0; $i < count( $expRecsArray ); $i++ )
      {
         $this -> assertEquals( $expRecsArray[$i]['b'], $actRecsArray[$i]['b'] -> __toString(), '$i = '.$i );
      }
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