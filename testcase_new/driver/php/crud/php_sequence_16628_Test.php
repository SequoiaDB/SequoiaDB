/****************************************************
@description:     test auto sequence
@testlink cases:   seqDB-16628
@modify list:
        2018-12-07 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestSequence16628 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs16628";
   private static $clName = "cl16628";
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
      echo "\n---Begin to test Sequence.\n";
      //$options, $expErrno, $expArray
      self::createSequence( null, -6, null );

      self::createSequence( array(), -6, null );

      self::createSequence( "", -258, null );

      self::createSequence( array( 'Field' => 'a' ), 0, array( 'a' ) );

      self::createSequence( "{ 'Field':'b' }" , 0, array( 'a', 'b' ) );

      self::insertAndCheckRecord();
      //$options, $expErrno, $expArray
      self::dropSequence( null, -6, array( 'a', 'b' ) );

      self::dropSequence( array( 'Field' => 'c' ), -333, array( 'a', 'b' ) );

      self::dropSequence( "", -258, array( 'a', 'b' ) );

      self::dropSequence( array(), -6, array( 'a', 'b' ) );

      self::dropSequence( array( 'Field' => 'a' ), 0, array( 'b' ) );

      self::dropSequence( "{ 'Field':'b' }" , 0, null );
      
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
      echo "\n---Test 16628 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
      
   }

   private static function createSequence( $options, $expErrno, $expArray )
   {
      self::$cl -> createAutoIncrement( $options );
      globalParameter::checkError( self::$db, $expErrno, "create autoIncrement error" );

      $clFullName = self::$cl -> getFullName();
      self::checkSequence( $clFullName, $expArray );
   }

   private static function dropSequence( $options, $expErrno, $expArray )
   {
      self::$cl -> dropAutoIncrement( $options );
      globalParameter::checkError( self::$db, $expErrno, "drop autoIncrement error");

      $clFullName = self::$cl -> getFullName();
      self::checkSequence( $clFullName, $expArray );
   }

   private static function checkSequence( $clFullName, $expArray )
   {
      $cursor = self::$db -> snapshot( SDB_SNAP_CATALOG, array( 'Name' => $clFullName ) );
      if( empty($cursor) )
      {
         throw new Exception( $clFullName . " snapshot doesn't exist." );
      }
      if( $expArray != null )
      {
         $AutoIncrements = $cursor -> next()['AutoIncrement'];
         $actArray = array();
         for ( $i = 0; $i < count( $AutoIncrements ); $i++ )
         {
            $actArray[ $i ] = $AutoIncrements[$i]["Field"];
         }
         sort( $expArray );
         sort( $actArray );
         if( $expArray !=  $actArray )
         {
            throw new Exception( "check the cl sequence error, exp: " . $expArray . "act: " . $actArray );
         }
      }
      else
      {
         $record = $cursor -> next();
         if( array_key_exists( "AutoIncrement", $record ) && count( $record["AutoIncrement"] ) != 0 )
         {
            var_dump( $record );
            throw new Exception( "AutoIncrement should not exist, but can found it AutoIncrement" );
         }
      }
   }
   
   private static function insertAndCheckRecord()
   {
      $recordNum = 10;
      for( $i = 0; $i < $recordNum; $i++ )
      {
         self::$cl -> insert( array( "str" => "test increment" . $i) );
      }
      globalParameter::checkError( self::$db, 0, "insert record");

      $cursor = self::$cl -> find( null, null, array( 'a' => 1 ) );
      if( empty( $cursor ) ) 
      {
         throw new Exception( "find error, cl is empty" );
      }
      $times = 1;
      while( $record = $cursor -> next() )
      {
         if( is_object( $record['a'] ) )
         {
            $resultA = $record['a'] -> __toString();
            $resultB = $record['b'] -> __toString();
         }else
         {
            $resultA = $record['a'];
            $resultB = $record['b'];
         }
         if( intval($resultA) != $times || intval($resultB) != $times )
         {
            throw new Exception( "check record error, exp: ". $times ." act: " ."a = ".$record['a'].", b = ".$record['b'] );
         }
         $times++;
      }
   }

}

?>
