/****************************************************
@description:     test analyze inexistent cl
@testlink cases:  seqDB-14240
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';

class TestAnalyzeCl14240 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = "analyze14240";
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$db -> createCS( self::$csName );
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
   }
   
   function test()
   {
      $inexistClFullName = 'analyze14240.foolish_name_xcvebcjd';
      $err = self::$db -> analyze( array( "Collection" => $inexistClFullName ) );
      analyzeUtils::checkErrno( -23, $err['errno'] );
   }
   
   public static function tearDownAfterClass()
   {
      self::$db -> dropCS( self::$csName );
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$db -> close();
   }
}
?>
