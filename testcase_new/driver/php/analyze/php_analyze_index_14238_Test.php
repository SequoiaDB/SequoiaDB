/****************************************************
@description:     test analyze index
@testlink cases:  seqDB-14238
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';

class TestAnalyzeIndex14238 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = "analyze14238";
   private static $clName = "analyze14238";
   private static $indexNum = 5;
   private static $cl;
   private static $analyzeIdx;
   private static $nonAnalyzeIdxArray;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );

      self::$cl = self::createCSCL( self::$db, self::$csName, self::$clName );
      $idxArray = array();
      self::insertData( self::$cl, self::$indexNum );
      for( $i = 0; $i < self::$indexNum; $i++ )
      {
         $fieldName = "field_".$i;
         $indexDef = array( $fieldName => 1 );
         $indexName = "idx_".$fieldName;
         $err = self::$cl -> createIndex( $indexDef, $indexName );
         analyzeUtils::checkErrno( 0, $err ['errno'] );
         array_push( $idxArray, $indexName );
      }
      self::$analyzeIdx = array_pop( $idxArray );
      self::$nonAnalyzeIdxArray = $idxArray;
   }
   
   function test()
   {
      self::checkScanTypeByExplain( self::$db, self::$cl, self::$analyzeIdx, "ixscan" );
      foreach( self::$nonAnalyzeIdxArray as $nonAnalyzeIdx )
         self::checkScanTypeByExplain( self::$db, self::$cl, $nonAnalyzeIdx, "ixscan" );

      $clFullName = self::$cl -> getFullName();
      analyzeUtils::checkErrno( 0, self::$db -> getError() ['errno'] );
      $option = array( "Collection" => $clFullName, "Index" => self::$analyzeIdx ); 
      $err = self::$db -> analyze( $option );
      analyzeUtils::checkErrno( 0, $err['errno'] );

      self::checkScanTypeByExplain( self::$db, self::$cl, self::$analyzeIdx, "tbscan" );
      foreach( self::$nonAnalyzeIdxArray as $nonAnalyzeIdx )
         self::checkScanTypeByExplain( self::$db, self::$cl, $nonAnalyzeIdx, "ixscan" );
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

   public static function insertData( $cl, $fieldNum )
   {
      $recs = array();
      $recNum = 20000;
      $doc = array();
      for( $i = 0; $i < $fieldNum; $i++ )
      {
         $fieldName = "field_".$i;
         $doc[$fieldName] = 0;
      }
      for( $i = 0; $i < $recNum; $i++ )
         array_push( $recs, $doc );
      $err = $cl -> bulkInsert( $recs, 0 );
      analyzeUtils::checkErrno( 0, $err ['errno'] );
   }

   public static function checkScanTypeByExplain( $db, $cl, $indexName, $expScanType )
   {
      $fieldName = substr( $indexName, 4, strlen( $indexName ) );
      $cond = array( $fieldName => 0 );
      $opt = array( "Run" => true );
      $cursor = $cl -> explain( $cond, /*selector*/null, /*orderBy*/null, /*hint*/null,
                              /*numToSkip*/0, /*numToReturn*/-1, /*flag*/0, $opt );
      analyzeUtils::checkErrno( 0, $db -> getError() ['errno'] );
      $rec = $cursor -> next();
      $actScanType = $rec['ScanType'];
      if( $actScanType != $expScanType )
      {
         $clName = $cl -> getName();
         analyzeUtils::checkErrno( 0, $db -> getError() ['errno'] );
         throw new Exception( "wrong scanType. cl: ". $clName
               ." expect: ". $expScanType ." actual: ". $actScanType );
      }
   }
}
?>
