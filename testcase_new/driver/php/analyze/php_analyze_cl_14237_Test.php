/****************************************************
@description:     test analyze cl
@testlink cases:  seqDB-14237
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';

class TestAnalyzeCl14237 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csNameBase = "analyze14237";
   private static $csNum = 2;
   private static $clNumPerCs = 2;
   private static $analyzeCl;
   private static $nonAnalyzeClArray;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      $clArray = self::createCSCL( self::$db, self::$csNameBase, self::$csNum, self::$clNumPerCs );
      foreach( $clArray as $cl )
         analyzeUtils::insertDataWithIndex( $cl );
      self::$analyzeCl = array_pop( $clArray );
      self::$nonAnalyzeClArray = $clArray;
   }
   
   function test()
   {
      analyzeUtils::checkScanTypeByExplain( self::$db, self::$analyzeCl, "ixscan" );
      foreach( self::$nonAnalyzeClArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "ixscan" );

      $clFullName = self::$analyzeCl -> getFullName();
      analyzeUtils::checkErrno( 0, self::$db -> getError() ['errno'] );
      $err = self::$db -> analyze( array( "Collection" => $clFullName ) );
      analyzeUtils::checkErrno( 0, $err['errno'] );

      analyzeUtils::checkScanTypeByExplain( self::$db, self::$analyzeCl, "tbscan" );
      foreach( self::$nonAnalyzeClArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "ixscan" );
   }
   
   public static function tearDownAfterClass()
   {
      self::dropCSCL( self::$db, self::$csNameBase, self::$csNum );
      self::$db -> close();
   }

   private static function createCSCL( $db, $csNameBase, $csNum, $clNumPerCs )
   {
      $clArray = array();
      for( $i = 0; $i < $csNum; $i++ )
      {
         $csName = $csNameBase ."_". $i;
         $partClArr = analyzeUtils::createCSCL( $db, $csName, $clNumPerCs );
         $clArray = array_merge( $clArray, $partClArr );
      }
      return $clArray;
   }

   private static function dropCSCL( $db, $csNameBase, $csNum )
   {
      for( $i = 0; $i < $csNum; $i++ )
      {
         $csName = $csNameBase ."_". $i;
         $db -> dropCS( $csName );
         analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      }
   }
}
?>
