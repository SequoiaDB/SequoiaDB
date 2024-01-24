/****************************************************
@description:     test auto sequence
@testlink cases:   seqDB-16652
@modify list:
        2018-12-07 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestSequence16652 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs16652";
   private static $clName = "cl16652";
   private static $cs;
   private static $cl;
   private static $db;
   private static $skipTest = false;
   private static $beginTime;
   private static $endTime;

   public function setUp()
   {
      date_default_timezone_set("Asia/Shanghai");
      self::$beginTime = microtime( true );
      echo "\n---Begin time: " . date( "Y-m-d H:i:s", self::$beginTime ) ."\n";
   
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      globalParameter::checkError( self::$db, 0);
      if( globalParameter::isStandalone( self::$db ) )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
      self::$cs = self::$db -> selectCS( self::$csName );
      globalParameter::checkError( self::$db, 0, "create cs error");
      self::$cl = self::$cs -> selectCL( self::$clName, array( 'AutoIncrement' => array( 'Field' => 'a', 'MaxValue' => 20000 ) ) );
      globalParameter::checkError( self::$db, 0, "create cl error");
      self::checkSnapshot( 20000 );
   }

   function test()
   {
      echo "\n---Begin to test bulkInsert.\n";
      self::$cl -> alter(  array( 'AutoIncrement' => array( 'Field' => 'a', 'MaxValue' => 1000 ) ) ) ;
      globalParameter::checkError( self::$db, 0, "alter cl error");
      self::checkSnapshot( 1000 );

      //insert 1000 record
      for( $i = 0; $i < 1000; $i++ )
      {
         self::$cl -> insert( array( 'c' => $i ) );
         globalParameter::checkError( self::$db, 0, "cl insert record error");
      }

      //insert more than 1000 records
      self::$cl -> insert( array( 'c' => 1001 ) );
      globalParameter::checkError( self::$db, -325, "insert record voer 1000");
   }
   
   public function tearDown()
   {
      $err = self::$db -> dropCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to drop cs, errno=".$err['errno']);
      }

      self::$db->close();
      
      self::$endTime = microtime( true );
      echo "\n---End the Test,End time: " . date( "Y-m-d H:i:s", self::$endTime ) . "\n";
      echo "\n---Test 16652 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
   private static function checkSnapshot( $maxValue )
   {
      $fullName = self::$csName . '.' . self::$clName;
      $cursor = self::$db -> snapshot( SDB_SNAP_CATALOG, array( 'Name' => $fullName ) );
      if( empty( $cursor ) )
      {
         throw new Exception( "check cl sequence error,is empty." );
      }
      $sequenceID;
      while( $record = $cursor -> next() )
      {
         $sequenceID = $record['AutoIncrement'][0]['SequenceID'];
      }
      $cursor  = self::$db -> snapshot( 15, array( 'ID' => $sequenceID) );
      if( empty( $cursor ) )
      {
         throw new Exception( "select sequence error,is empty." );
      }
      $actMaxValue = $cursor -> next()['MaxValue'];
      if( is_object( $actMaxValue ) )
      {
         $actMaxValue = $actMaxValue -> __toString();
      }
      if( $maxValue != intval($actMaxValue) )
      {
         throw new Exception( "check sequence maxValue, exp: ". $maxValue ." act: " . $actMaxValue );
      }
   }

}

?>
