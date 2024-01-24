/****************************************************
@description:     test analyze getIndexStat
@testlink cases:  seqDB-24685
@modify list:
      2021-11-25  Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';

class TestAnalyzeGetIndexStat24685 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = "analyze24685";
   private static $clName = "analyze24685";
   private static $indexName = "index24685";
   private static $recNum = 20000;
   private static $cl;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );

      self::$cl = self::createCSCL( self::$db, self::$csName, self::$clName );
      self::insertData( self::$cl, self::$recNum );
      $indexDef = array( "field_1" => 1 );
      $err = self::$cl -> createIndex( $indexDef, self::$indexName );
      analyzeUtils::checkErrno( 0, $err ['errno'] );
      self::$db -> analyze( array( 'Collection' => self::$csName.'.'.self::$clName ) );
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
   }
   
   function test()
   {
      $rec = self::$cl -> getIndexStat( self::$indexName );
      analyzeUtils::checkErrno( 0, self::$db -> getError() ['errno'] );
      $actCollection = $rec['Collection'];
      $this -> assertEquals( self::$csName.'.'.self::$clName, $actCollection );
      $actIndex = $rec['Index'];
      $this -> assertEquals( self::$indexName, $actIndex );
      $actTotalRecords = $rec['TotalRecords'];
      $this -> assertEquals( self::$recNum, $actTotalRecords );
      
      $notExistIndexName = "indexNotExist24685";
      $rec = self::$cl -> getIndexStat( $notExistIndexName );
      analyzeUtils::checkErrno( -356, self::$db -> getError() ['errno'] );
   }
   
   public static function tearDownAfterClass()
   {
      $err = self::$db -> dropCS( self::$csName );
      analyzeUtils::checkErrno( 0, $err['errno'] );
      self::$db -> close();
   }

   private static function createCSCL( $db, $csName, $clName )
   {
      $db -> createCS( $csName );
      analyzeUtils::checkErrno( 0, $db -> getError()['errno'] );
      $cs = $db -> getCS( $csName );
      analyzeUtils::checkErrno( 0, $db -> getError()['errno'] );
      $cs -> createCL( $clName );
      analyzeUtils::checkErrno( 0, $db -> getError()['errno'] );
      $cl = $cs -> getCL( $clName );
      analyzeUtils::checkErrno( 0, $db -> getError()['errno'] );
      return $cl;
   }

   public static function insertData( $cl, $recNum  )
   {
      $recs = array();
      
      $doc = array();
      for( $i = 0; $i < 5; $i++ )
      {
         $fieldName = "field_".$i;
         $doc[$fieldName] = 0;
      }
      for( $i = 0; $i < $recNum; $i++ )
         array_push( $recs, $doc );
      $err = $cl -> bulkInsert( $recs, 0 );
      analyzeUtils::checkErrno( 0, $err ['errno'] );
   }
}
?>
