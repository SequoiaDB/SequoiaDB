/****************************************************
@description:      update, cover all update character
@testlink cases:   seqDB-7831
@input:        1 createCL
               2 insert
               3 update, cover all update character:
                 $inc/$set/$unset/$addtoset/$pop/$pull/$pull_all/$push/$push_all/$replace
               6 dropCL
@output:     success
@modify list:
        2016-4-22 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class UpdateSymbol extends BaseOperator 
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
      $recs1 = '{a: 0, int: 1, str: "test", arr: [1,2,3,4], tmp:"" }';
      $recs2 = '{a: 1, int: 2 }';
      $clDB -> insert( $recs1 );
      $clDB -> insert( $recs2 );
   }
   
   function update( $clDB )
   {
      //$inc 
      $clDB -> update( '{$inc: {int: 1} }',       '{a:0}' );  //{int:1} -> {int:2}
      //$set
      $clDB -> update( '{$set: {str: "hello"} }', '{a:0}' );  //{str:"test"} -> {str:"hello"}
      //$unset
      $clDB -> update( '{$unset: {tmp: ""} }',    '{a:0}' );  //{tmp:""} -> a is not exist
      //$addtoset
      $clDB -> update( '{$addtoset: {arr: [1,5]} }',   '{a:0}' );  //{arr:[1,2,3,4]} -> {arr:[1,2,3,4,5]}
      //$pop
      $clDB -> update( '{$pop: {arr: -1} }',           '{a:0}' );  //{arr:[1,2,3,4,5]} -> {arr:[2,3,4,5]}
      //$pull
      $clDB -> update( '{$pull: {arr: 3} }',           '{a:0}' );  //{arr:[2,3,4,5]} -> {arr:[2,4,5]}
      //$pull_all
      $clDB -> update( '{$pull_all: {arr: [2, 20]} }', '{a:0}' );  //{arr:[2,4,5]} -> {arr:[4,5]}
      //$push
      $clDB -> update( '{$push: {arr: 6} }',           '{a:0}' );  //{arr:[4,5]} -> {arr:[4,5,6]}
      //$push_all
      $clDB -> update( '{$push_all: {arr: [4, 7]} }',  '{a:0}' );  //{arr:[4,5,6]} -> {arr:[4,5,6,4,7]}
      //$replace 
      $clDB -> update( '{$replace: {rep: 3} }',        '{a:1}' );  //{a:1, int:2} -> {rep:3}
   }
   
   function findRecs( $clDB )
   {
      $orderby   = array( 'a' => 1 );
      $cursor = $clDB -> find( null, null, $orderby );
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
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestUpdateSymbol extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new UpdateSymbol();
      
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
   
   function test_update()
   {
      echo "\n   Begin to update records.\n";
      
      self::$dbh -> update( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $findRecsArray = self::$dbh -> findRecs( self::$clDB );
      //compare exec result
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //compare for $replace
      $this -> assertEquals( 3, $findRecsArray[0]['rep'] );
      $this -> assertArrayNotHasKey( "a", $findRecsArray[0] );
      $this -> assertArrayNotHasKey( "int", $findRecsArray[0] );
      //compare for others
      $this -> assertEquals( 0, $findRecsArray[1]['a'] );
      $this -> assertEquals( 2, $findRecsArray[1]['int'] );
      $this -> assertEquals( 4, $findRecsArray[1]['arr'][0] );
      $this -> assertEquals( 5, $findRecsArray[1]['arr'][1] );
      $this -> assertEquals( 6, $findRecsArray[1]['arr'][2] );
      $this -> assertEquals( 4, $findRecsArray[1]['arr'][3] );
      $this -> assertEquals( 7, $findRecsArray[1]['arr'][4] );
      $this -> assertCount( 5, $findRecsArray[1]['arr'] );
      $this -> assertEquals( 'hello', $findRecsArray[1]['str'] );
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