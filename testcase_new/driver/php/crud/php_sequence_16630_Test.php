/****************************************************
@description:     test auto sequence
@testlink cases:   seqDB-16630
@modify list:
        2018-12-07 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestSequence16630 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs16630";
   private static $clName = "cl16630";
   private static $fullName = "cs16630.cl16630";
   private static $sequenceName;
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
      self::$cl = self::$cs -> selectCL( self::$clName );
      globalParameter::checkError( self::$db, 0, "create cl error");
   }

   function test()
   {
      echo "\n---Begin to test bulkInsert.\n";
      $err = self::$cl -> createAutoIncrement( array( 'Field' => 'a', 'MaxValue' => 4096, 'MinValue' => 1024, 'StartValue' => 2048 ) );
      globalParameter::checkError( self::$db, 0, "create autoInrement error");

      self::checkSnapshot();

      self::checkList(); 
      
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
      echo "\n---Test 16630 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
   }
   
   private static function checkSnapshot( )
   {  
      $cursor = self::$db -> snapshot( SDB_SNAP_CATALOG, array( 'Name' => self::$fullName ) );
      if( empty( $cursor ) )
      {  
         throw new Exception( "check cl sequence error,is empty." );
      }
      $sequenceID;
      while( $record = $cursor -> next() )
      {  
         $sequenceID = $record['AutoIncrement'][0]['SequenceID'];
         self::$sequenceName = $record['AutoIncrement'][0]['SequenceName'];
      }
      $cursor  = self::$db -> snapshot( 15, array( 'ID' => $sequenceID) );
      if( empty( $cursor ) )
      {  
         throw new Exception( "select sequence error,is empty." );
      }
      $record  = $cursor -> next();
      if( is_object( $record['Version'] ) )
      {
         $version = $record['Version'] -> __toString();
         $currentValue = $record['CurrentValue'] -> __toString();
         $startValue = $record['StartValue'] -> __toString();
         $minValue = $record['MinValue'] -> __toString();
         $maxValue = $record['MaxValue'] -> __toString();
      }
      else
      {
         $version = $record['Version'];
         $currentValue = $record['CurrentValue'];
         $startValue = $record['StartValue'];
         $minValue = $record['MinValue'];
         $maxValue = $record['MaxValue'];
      }
      self::checkSnapshotValue( $record['Name'], self::$sequenceName );
      self::checkSnapshotValue( $record['Internal'], true );
      self::checkSnapshotValue( $version, '0');
      self::checkSnapshotValue( intval( $currentValue ), 2048 );
      self::checkSnapshotValue( intval( $startValue ), 2048 );
      self::checkSnapshotValue( intval( $minValue ), 1024 );
      self::checkSnapshotValue( intval( $maxValue ), 4096 );
      self::checkSnapshotValue( $record['Increment'], 1 );
      self::checkSnapshotValue( $record['CacheSize'], 1000 );
      self::checkSnapshotValue( $record['AcquireSize'], 1000 );
      self::checkSnapshotValue( $record['Cycled'], false );
      self::checkSnapshotValue( $record['Initial'], true );
      
   }

   private static function checkList()
   {
      $cursor = self::$db -> list( SDB_SNAP_SEQUENCES, array( 'Name' => self::$sequenceName ));
      if( empty( $cursor ) ) 
      {
         throw new Exception( 'list sequence error, is empty.');
      }
   }

   private static function checkSnapshotValue( $expValue, $actValue )
   {
      if( $expValue != $actValue )
      {
         throw new Exception( 'check sequence snapshot error, exp: ' .$expValue . ' act: ' .$actValue );
      }
   }

}

?>
