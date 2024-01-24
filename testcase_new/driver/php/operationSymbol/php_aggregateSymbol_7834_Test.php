/****************************************************
@description:      aggregate, cover all selector character
@testlink cases:   seqDB-7833
@input:        1 createCL
               2 insert
               3 aggregate, cover all group's character:
                 $addtoset.....................$count
                 info: others aggregate char @php_aggregate_base_paramString_7679.php
               6 dropCL
@output:     success
@modify list:
        2016-5-3 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class AggregateSymbol extends BaseOperator 
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
      $recs1 = '{a: 1, b: 1}';
      $recs2 = '{a: 1, b: 3}';
      $recs3 = '{a: 2, b: 7}';
      $recs4 = '{a: 2, b: 9}';
      $clDB -> insert( $recs1 );
      $clDB -> insert( $recs2 );
      $clDB -> insert( $recs3 );
      $clDB -> insert( $recs4 );
   }
   
   function aggregate( $clDB )
   {
      $aggrObj = array( '{$group:{_id:"$a",
                           fst:{$first:"$a"},
                           lst:{$last:"$b"},
                           adt:{$addtoset:"$b"},
                           max:{$max:"$b"},
                           min:{$min:"$b"},
                           avg:{$avg:"$b"},
                           sum:{$sum:"$b"},
                           cnt:{$count:"$a"},
                           push:{$push:"$b"}}}', 
                           '{$sort:{fst:1}}' );
      $cursor = $clDB -> aggregate( $aggrObj );
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
      return $findRecsArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestAggreSymbol extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new AggregateSymbol();
      
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
   
   function test_aggregate()
   {
      echo "\n---Begin to exec aggregate.\n";
      $tmpArray = self::$dbh -> aggregate( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      //$first
      $this -> assertEquals( 1, $tmpArray[0]['fst'] );
      $this -> assertEquals( 2, $tmpArray[1]['fst'] );
      //$last
      $this -> assertEquals( 3, $tmpArray[0]['lst'] );
      $this -> assertEquals( 9, $tmpArray[1]['lst'] );
      //$adt
      $this -> assertEquals( 1, $tmpArray[0]['adt'][0] );
      $this -> assertEquals( 3, $tmpArray[0]['adt'][1] );
      $this -> assertEquals( 7, $tmpArray[1]['adt'][0] );
      $this -> assertEquals( 9, $tmpArray[1]['adt'][1] );
      //$max
      $this -> assertEquals( 3, $tmpArray[0]['max'] );
      $this -> assertEquals( 9, $tmpArray[1]['max'] );
      //$min
      $this -> assertEquals( 1, $tmpArray[0]['min'] );
      $this -> assertEquals( 7, $tmpArray[1]['min'] );
      //$avg
      $this -> assertEquals( 2, $tmpArray[0]['avg'] );
      $this -> assertEquals( 8, $tmpArray[1]['avg'] );
      //$cnt
      if( is_object( $tmpArray[0]['cnt'] ) )
      {
         $this -> assertEquals( "2", $tmpArray[0]['cnt'] -> __toString() );
         $this -> assertEquals( "2", $tmpArray[1]['cnt'] -> __toString() );
      }
      else
      {
         $this -> assertEquals( "2", $tmpArray[0]['cnt'] );
         $this -> assertEquals( "2", $tmpArray[1]['cnt'] );   
      }
      //$push
      $this -> assertEquals( 1, $tmpArray[0]['push'][0] );
      $this -> assertEquals( 3, $tmpArray[0]['push'][1] );
      $this -> assertEquals( 7, $tmpArray[1]['push'][0] );
      $this -> assertEquals( 9, $tmpArray[1]['push'][1] );
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