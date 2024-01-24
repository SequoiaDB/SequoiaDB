/****************************************************
@description:     test timestamp increment
@testlink cases:   seqDB-16646
@modify list:
        2018-11-19 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestTimestamp16646 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs16646";
   private static $clName = "cl16646";
   private static $cs;
   private static $cl;
   private static $db;
   private static $beginTime;
   private static $endTime;

   public static function setUpBeforeClass()
   {
      date_default_timezone_set("Asia/Shanghai");
      self::$beginTime = microtime( true );
      echo "\n---Begin time: " . date( "Y-m-d H:i:s", self::$beginTime ) ."\n";
      
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$cs = self::$db -> selectCS( self::$csName );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$cl = self::$cs -> selectCL( self::$clName );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
   }

   function test()
   {
      echo "\n---Begin to test timestamp.\n";

      $maxTime = 2147483647;
      $minTime = -2147483648;

      //check insert inc>999999 timestamp
      self::$cl -> insert( array( 'time' => new SequoiaTimestamp( $maxTime, -1000000 ), 'No' => 1) );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkIncResult( new SequoiaTimestamp( 2147483646 ), 1);

      self::$cl -> insert( array( 'time' => new SequoiaTimestamp( $maxTime, 1000000 ), 'No' => 2) );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkIncResult( new SequoiaTimestamp( -2147483648 ), 2);

      self::$cl -> insert( array( 'time' => new SequoiaTimestamp( $minTime, -1000000 ), 'No' => 3) );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkIncResult( new SequoiaTimestamp( 2147483647 ), 3);

      self::$cl -> insert( array( 'time' => new SequoiaTimestamp( $minTime, -1000000 ), 'No' => 4) );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkIncResult( new SequoiaTimestamp( 2147483647 ), 4);
      
      //check update inc>999999 timestamp
      self::$cl -> update( array( '$set' => array( 'time' => new SequoiaTimestamp( $maxTime, 1000000 ) ) ), array( 'No' => 1 ) );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkIncResult( new SequoiaTimestamp( -2147483648 ), 1);

      self::$cl -> update( array( '$set' => array( 'time' => new SequoiaTimestamp( $minTime, -1000000 ) ) ), array( 'No' => 4 ) );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkIncResult( new SequoiaTimestamp( 2147483647 ), 4);
   }
   
   public static function tearDownAfterClass()
   {
      $err = self::$db -> dropCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to drop cs, errno=".$err['errno']);
      }

      self::$db->close();
      
      self::$endTime = microtime( true );
      echo "\n---End the Test,End time: " . date( "Y-m-d H:i:s", self::$endTime ) . "\n";
      echo "\n---Test 16646 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }

   function checkIncResult( $times, $No)
   {
      $cursor = self::$cl -> find( array( 'No' => $No ));
      if( empty( $cursor ))
      {
          throw new Exception( " no find record where No = ".$No );
      }
      while( $record = $cursor -> next() )
      {
         $actTime = $record[ 'time'];
         if( $actTime != $times -> __toString() )
         {
            throw new Exception( "check " . $No . " value error, exp: " . $times ." act: " . $actTime );
         }
      }
   }

   private static function checkErrno( $expErrno, $actErrno, $msg = "" )
   {
      if( $expErrno != $actErrno )
      {
         throw new Exception( "expect [".$expErrno."] but found [".$actErrno."]. ".$msg );
      }
   }
}

?>
