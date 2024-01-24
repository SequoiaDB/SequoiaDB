/****************************************************
@description:      find, cover all selector character
@testlink cases:   seqDB-7833
@input:        1 createCL
               2 insert
               3 find, cover all selector character:
                 $include_once.....................$cast
               6 dropCL
@output:     success
@modify list:
        2016-5-3 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SelectSymbol extends BaseOperator 
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
      $recs1 = '{int: -1,  str: "test",  arr: [1,2], tmp: 3, trm: " world ", d: 0}';
      $recs2 = '{int: 2.6, str: "HELLO", arr: [{a:1}], nu: null, d: 1}';
      $clDB -> insert( $recs1 );
      $clDB -> insert( $recs2 );
   }
   
   function findRecs( $clDB, $selecType )
   {
      $orderby = '{d:1}';
      
      // $include
      if( $selecType === '$include' ){ $selector = '{int:{$include:0}}'; }   //return 'int is not exist'
      // $default
      if( $selecType === '$default' ){ $selector = '{def:{$default:"defv"}}'; } //return 'def: defv'
      // $elemMatch
      if( $selecType === '$elemMatch'    ){ $selector = '{arr:{$elemMatch:{a:1}}}'; }    //return 'arr: [{a:1}]'
      // $elemMatchOne
      if( $selecType === '$elemMatchOne' ){ $selector = '{arr:{$elemMatchOne:{a:1}}}'; } //return 'arr: [{a:1}]'
      // $slice
      if( $selecType === '$slice' ){ $selector = '{arr:{$slice:1}}'; }  //return 'arr: [{a:1}]'
      // $abs
      if( $selecType === '$abs'     ){ $selector = '{int:{$abs:1}}'; }     //return 'int: 1 / int: 2.6'
      // $ceiling
      if( $selecType === '$ceiling' ){ $selector = '{int:{$ceiling:1}}'; } //return 'int: -1 / int: 2'
      // $floor
      if( $selecType === '$floor'   ){ $selector = '{int:{$floor:1}}'; }   //return 'int: -1 / int: 3'
      // $mod
      if( $selecType === '$mod'      ){ $selector = '{int:{$mod:2}}'; }  //return 'int: -1 / int: 0.6'
      // $add
      if( $selecType === '$add'      ){ $selector = '{int:{$add:10}}'; } //return 'int: 9 / int: 12.6'
      // $subtract
      if( $selecType === '$subtract' ){ $selector = '{int:{$subtract:10}}'; }  //return 'int: -11 / int: -8.4'
      // $multiply
      if( $selecType === '$multiply' ){ $selector = '{int:{$multiply:10}}'; }  //return 'int: -10 / int: 26'
      // $divide
      if( $selecType === '$divide'   ){ $selector = '{int:{$divide:10}}'; }  //return 'int: -0.1 / int: 0.26'
      // $substr  
      if( $selecType === '$substr' ){ $selector = '{str:{$substr:[2,1]}}'; } //return 'str: "s" / str: "L"'
      // $strlen
      if( $selecType === '$strlen' ){ $selector = '{str:{$strlen:1}}'; }    //return 'str: 4 / str: 5'
      // $lower
      if( $selecType === '$lower' ){ $selector = '{str:{$lower:1}}'; }  //return 'str: "test" / str: "hello"'
      // $upper
      if( $selecType === '$upper' ){ $selector = '{str:{$upper:1}}'; }  //return 'str: "TEST" / str: "HELLO"'
      // $ltrim
      if( $selecType === '$ltrim' ){ $selector = '{trm:{$ltrim:1}}'; }  //return 'trm: "world "'
      // $rtrim
      if( $selecType === '$rtrim' ){ $selector = '{trm:{$rtrim:1}}'; }  //return 'trm: " world"'
      // $trim
      if( $selecType === '$trim'  ){ $selector = '{trm:{$trim: 1}}'; }  //return 'trm: "world"'
      // $cast
      if( $selecType === '$cast'  ){ $selector = '{int:{$cast:"string"}}'; } //return 'int: "-1" / int: "2.6"'
      
      $cursor = $clDB -> find( null, $selector, $orderby );
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to find records using ". $selecType .". Errno: ". $errno ."\n";
      }
      
      $findRecsArray = array() ;
      while( $recs = $cursor -> next() )
      {
         array_push( $findRecsArray, $recs );
      }
      // echo "\n--------use \"". $selecType ."\"------".$selector."----------\n";
      // var_dump($findRecsArray);
      return $findRecsArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestSelectSymbol extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SelectSymbol();
      
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
      // $include
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$include' );
      $this -> assertCount( 2, $findRecsArray );
      $this -> assertArrayNotHasKey( "int", $findRecsArray[0] );
      $this -> assertArrayNotHasKey( "int", $findRecsArray[1] );
      // $default
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$default' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( "defv", $findRecsArray[0]['def'] );
      $this -> assertEquals( "defv", $findRecsArray[1]['def'] );
      // $elemMatch
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$elemMatch' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertCount ( 0, $findRecsArray[0]['arr'] );
      $this -> assertEquals( 1, $findRecsArray[1]['arr'][0]['a'] ); 
      // $elemMatchOne
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$elemMatchOne' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertCount ( 1, $findRecsArray[1]['arr'] );
      $this -> assertEquals( 1, $findRecsArray[1]['arr'][0]['a'] ); 
      // $slice
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$slice' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['arr'][0] );
      $this -> assertEquals( 1, $findRecsArray[1]['arr'][0]['a'] );
      // $abs
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$abs' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( 1, $findRecsArray[0]['int'] );
      $this -> assertEquals( 2.6, $findRecsArray[1]['int'] );
      // $ceiling
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$ceiling' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals(  -1, $findRecsArray[0]['int'] );
      $this -> assertEquals( 3.0, $findRecsArray[1]['int'] );
      // $floor
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$floor' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals(  -1, $findRecsArray[0]['int'] );
      $this -> assertEquals( 2.0, $findRecsArray[1]['int'] );
      // $mod
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$mod' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals(  -1, $findRecsArray[0]['int'] );
      $this -> assertEquals( 0.6, $findRecsArray[1]['int'] );
      // $add
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$add' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals(    9, $findRecsArray[0]['int'] );
      $this -> assertEquals( 12.6, $findRecsArray[1]['int'] );
      // $subtract
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$subtract' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals(  -11, $findRecsArray[0]['int'] );
      $this -> assertEquals( -7.4, $findRecsArray[1]['int'] );
      // $multiply
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$multiply' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( -10, $findRecsArray[0]['int'] );
      $this -> assertEquals(  26, $findRecsArray[1]['int'] );
      // $divide
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$divide' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( 0, $findRecsArray[0]['int'] );
      $this -> assertEquals( 0.26, $findRecsArray[1]['int'] );
      // $substr
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$substr' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( "s", $findRecsArray[0]['str'] );
      $this -> assertEquals( "L", $findRecsArray[1]['str'] );
      // $strlen
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$strlen' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( 4, $findRecsArray[0]['str'] );
      $this -> assertEquals( 5, $findRecsArray[1]['str'] );
      // $lower
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$lower' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( "test",  $findRecsArray[0]['str'] );
      $this -> assertEquals( "hello", $findRecsArray[1]['str'] );
      // $upper
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$upper' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( "TEST",  $findRecsArray[0]['str'] );
      $this -> assertEquals( "HELLO", $findRecsArray[1]['str'] );
      // $ltrim
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$ltrim' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( "world ", $findRecsArray[0]['trm'] );
      
      // $rtrim
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$rtrim' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( " world", $findRecsArray[0]['trm'] );
      // $trim
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$trim' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( "world",  $findRecsArray[0]['trm'] );
      // $cast
      $findRecsArray = self::$dbh -> findRecs( self::$clDB, '$cast' );
      $this -> assertCount ( 2, $findRecsArray );
      $this -> assertEquals( "-1",  $findRecsArray[0]['int'] );
      $this -> assertEquals( "2.6",  $findRecsArray[1]['int'] );
      
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