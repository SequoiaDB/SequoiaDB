/****************************************************
@description:     test analyze cs
@testlink cases:  seqDB-14236
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';

class TestAnalyzeCs14236 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $analyzeCsName = "analyzeCs14236";
   private static $nonAnalyzeCsName = "nonAnalyzeCs14236";
   private static $clNumPerCs = 2;
   private static $analyzeClArray;
   private static $nonAnalyzeClArray;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$analyzeClArray = analyzeUtils::createCSCL( self::$db, self::$analyzeCsName, self::$clNumPerCs );
      foreach( self::$analyzeClArray as $cl )
         analyzeUtils::insertDataWithIndex( $cl );
      self::$nonAnalyzeClArray = analyzeUtils::createCSCL( self::$db, self::$nonAnalyzeCsName, self::$clNumPerCs );
      foreach( self::$nonAnalyzeClArray as $cl )
         analyzeUtils::insertDataWithIndex( $cl );
   }
   
   function test()
   {
      foreach( self::$analyzeClArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "ixscan" );
      foreach( self::$nonAnalyzeClArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "ixscan" );

      $err = self::$db -> analyze( array( "CollectionSpace" => self::$analyzeCsName ) );
      analyzeUtils::checkErrno( 0, $err['errno'] );

      foreach( self::$analyzeClArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "tbscan" );
      foreach( self::$nonAnalyzeClArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "ixscan" );
   }
   
   public static function tearDownAfterClass()
   {
      $err = self::$db -> dropCS( self::$analyzeCsName );
      analyzeUtils::checkErrno( 0, $err['errno'] );
      $err = self::$db -> dropCS( self::$nonAnalyzeCsName );
      analyzeUtils::checkErrno( 0, $err['errno'] );
      self::$db -> close();
   }
}
?>
