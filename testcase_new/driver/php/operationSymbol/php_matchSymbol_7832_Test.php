/****************************************************
@description:      find, cover all update character
@testlink cases:   seqDB-7832
@input:        1 createCL
               2 insert
               3 find, cover all match character:
                 $gt/$gte/$lt/$lte.....................$field
               6 dropCL
@output:     success
@modify list:
        2016-5-3 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class MatchSymbol extends BaseOperator 
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
      $recs1 = '{int: 0, str: "test", arr: [1,2], tmp: 3, nu: "" }';
      $recs2 = '{int: 1, nu: null, subobj: {a: 1}}';
      $clDB -> insert( $recs1 );
      $clDB -> insert( $recs2 );
   }
   
   function findRecs( $clDB, $mathType )
   {
      // $and/$gt/$lt
      if( $mathType === '$and' ){ $condition = '{$and:[{int:{$gt:0}},{int:{$lt:2}}]}'; }   //return 'int: 1'
      // $not/$gte/$lte
      if( $mathType === '$not' ){ $condition = '{$not:[{int:{$lte:0}},{int:{$gte:0}}]}'; } //return 'int: 1'
      // $or/$ne/$et
      if( $mathType === '$or'  ){ $condition = '{$or:[{int:{$ne:0}},{int:{$et:1}}]}'; }    //return 'int: 1'
      // $mod
      if( $mathType === '$mod' ){ $condition = '{int:{$mod:[2,1]}}'; }  //return 'int: 1'
      // $in
      if( $mathType === '$in'  ){ $condition = '{int:{$in: [1,2]}}'; }  //return 'int: 1'
      // $nin
      if( $mathType === '$nin' ){ $condition = '{int:{$nin:[0,2]}}'; }  //return 'int: 1'
      // $isnull
      if( $mathType === '$isnull' ){ $condition = '{nu:{$isnull:1}}'; }    //return 'int: 1, nu: null'
      // $all
      if( $mathType === '$all'    ){ $condition = '{arr:{$all:[1,2]}}'; }  //return 'arr: [1,2]'
      // $type
      if( $mathType === '$type'   ){ $condition = '{nu:{$type:1,$et:10}}'; }   //return 'nu: null'
      // $exists
      if( $mathType === '$exists' ){ $condition = '{tmp:{$exists:1}}'; }  //return 'tmp: 3'
      // $elemMatch
      if( $mathType === '$elemMatch' ){ $condition = '{subobj:{$elemMatch:{a:1}}}'; }  //return 'subobj: {a:1}'
      // $+char
      if( $mathType === '$+'     ){ $condition = '{"arr.$1":1}'; }    //return 'arr: [1,2]'
      // $size
      if( $mathType === '$size'  ){ $condition = '{arr:{$size:1,$et:2}}'; }  //return 'arr: [1,2]'
      // $regex  
      if( $mathType === '$regex' ){ $condition = '{str:{$regex:"t.*st",$options:"i"}}'; }  //return 'str: "test"'
      // $field
      if( $mathType === '$field' ){ $condition = '{int:{$lt:{$field:"tmp"}}}'; }  //return 'int: 1'
      
      $cursor = $clDB -> find( $condition );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to find records using ". $mathType .". Errno: ". $errno ."\n";
      }
      
      $findRecsArray = array() ;
      while( $recs = $cursor -> next() )
      {
         array_push( $findRecsArray, $recs );
      }
      return $findRecsArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestMatchSymbol extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new MatchSymbol();
      
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
      // $and/$gt/$lt
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$and' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      // $not/$gte/$lte
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$not' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      // $or/$ne/$et
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$or' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      // $mod
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$mod' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      // $in
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$in' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      // $nin
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$in' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      // $isnull
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$isnull' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      $this -> assertNull( $findRecsArray[0]['nu'] );
      // $all
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$all' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['arr'][0] );
      $this -> assertEquals( 2, $findRecsArray[0]['arr'][1] );
      // $type
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$type' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertNull( $findRecsArray[0]['nu'] );
      // $exists
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$exists' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 3, $findRecsArray[0]['tmp'] );
      // $elemMatch
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$elemMatch' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['subobj']['a'] ); 
      // $+char 
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$+' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['arr'][0] );
      $this -> assertEquals( 2, $findRecsArray[0]['arr'][1] ); 
      // $size
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$size' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['arr'][0] );
      $this -> assertEquals( 2, $findRecsArray[0]['arr'][1] );
      // $regex
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$regex' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 'test', $findRecsArray[0]['str'] );
      // $field
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$field' );
      $this -> assertCount ( 1, $findRecsArray );
      $this -> assertEquals( 0, $findRecsArray[0]['int'] );
      $this -> assertEquals( 3, $findRecsArray[0]['tmp'] );
      
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