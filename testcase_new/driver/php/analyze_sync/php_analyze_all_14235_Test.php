/****************************************************
@description:     test global analyze 
@testlink cases:  seqDB-14235
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';

class TestAnalyzeAll14235 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csNameBase = "analyze14235";
   private static $csNum = 2;
   private static $clNumPerCs = 2;
   private static $clArray;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$clArray = self::createCSCL( self::$db, self::$csNameBase, self::$csNum, self::$clNumPerCs );
      foreach( self::$clArray as $cl )
         analyzeUtils::insertDataWithIndex( $cl );
   }
   
   function test()
   {
      foreach( self::$clArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "ixscan" );

      $err = self::$db -> analyze();
      analyzeUtils::checkErrno( 0, $err['errno'] );

      foreach( self::$clArray as $cl )
         analyzeUtils::checkScanTypeByExplain( self::$db, $cl, "tbscan" );
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
