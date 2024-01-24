/****************************************************
@description:     test analyze group
@testlink cases:  seqDB-14239
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';

class TestAnalyzeGroup14239 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = "analyze14239";
   private static $clName = "analyze14239";
   private static $cl;
   private static $analyzeGroup;
   private static $nonAnalyzeGroup;
   private static $analyzeRec;
   private static $nonAnalyzeRec;
   private static $skipTest;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );

      if( analyzeUtils::isStandAlone( self::$db ) 
            || sizeof( analyzeUtils::getDataGroups( self::$db ) ) < 2 )
      {
         echo "can not split, skip testcase\n";
         self::$skipTest = true;
         return ;
      }
      else
         self::$skipTest = false;
      $groupNames = analyzeUtils::getDataGroups( self::$db );
      self::$analyzeGroup = $groupNames[0];
      self::$nonAnalyzeGroup = $groupNames[1];

      self::$db -> createCS( self::$csName );
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      $cs = self::$db -> getCS( self::$csName );
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );

      $option = array
      ( 
         "ShardingKey" => array( "a" => 1 ), 
         "ShardingType" => "range" , 
         "Group" => self::$analyzeGroup 
      );
      $cs -> createCL( self::$clName, $option );
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$cl = $cs -> getCL( self::$clName );
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );

      $err = self::$cl -> split( self::$analyzeGroup, self::$nonAnalyzeGroup, 
            array( 'a' => 1000 ), array( 'a' => 3000 ) );
      analyzeUtils::checkErrno( 0, $err['errno'] );

      self::$analyzeRec = array( 'a' => 0 ); // records on analyzeGroup
      self::insertData( self::$cl, self::$analyzeRec );
      self::$nonAnalyzeRec = array( 'a' => 2000 ); // records on nonAnalyzeGroup
      self::insertData( self::$cl, self::$nonAnalyzeRec );
   }
   
   function test()
   {
      if( self::$skipTest ) return ;
      self::checkScanTypeByExplain( self::$db, self::$cl, self::$analyzeRec, "ixscan" );
      self::checkScanTypeByExplain( self::$db, self::$cl, self::$nonAnalyzeRec, "ixscan" );

      $option = array( 'GroupName' => self::$analyzeGroup ); 
      $err = self::$db -> analyze( $option );
      analyzeUtils::checkErrno( 0, $err['errno'] );

      self::checkScanTypeByExplain( self::$db, self::$cl, self::$analyzeRec, "tbscan" );
      self::checkScanTypeByExplain( self::$db, self::$cl, self::$nonAnalyzeRec, "ixscan" );
   }
   
   public static function tearDownAfterClass()
   {
      if( self::$skipTest ) return ;
      $err = self::$db -> dropCS( self::$csName );
      analyzeUtils::checkErrno( 0, $err['errno'] );
      self::$db -> close();
   }

   public static function insertData( $cl, $doc )
   {
      $recs = array();
      $recNum = 30000;
      for( $i = 0; $i < $recNum; $i++ )
         array_push( $recs, $doc );
      $err = $cl -> bulkInsert( $recs, 0 );
      analyzeUtils::checkErrno( 0, $err['errno'] );
   }

   public static function checkScanTypeByExplain( $db, $cl, $analyzeRec, $expScanType )
   {
      $cond = $analyzeRec;
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
