/****************************************************
@description:     test timestamp increment
@testlink cases:   seqDB-16775
@modify list:
        2018-11-19 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestCURDFlag16775 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs16775";
   private static $clName = "cl16775";
   private static $cs;
   private static $cl;
   private static $db;
   private static $data;
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
      
      self::$data = self::buildData();
      
      self::createUniqueIndex();
   }

   function test()
   {
      echo "\n---Begin to test bulkInsert.\n";
      
      //bulkInsert not exist record set flag=0
      self::$cl -> bulkInsert( self::$data, 0 );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkRecordNum( count(self::$data), "first bulkInsert record" );

      //bulkInsert already exist record set flag=0
      self::$cl -> bulkInsert( self::$data, 0 );
      self::checkErrno( -38, self::$db -> getError()['errno'] );
      self::checkRecordNum( count(self::$data), "bulkInsert the same record" );
      
      self::$data[ count(self::$data) ] = array( "test" => "test part record not exist");
      self::$cl -> bulkInsert( self::$data, SDB_FLG_INSERT_CONTONDUP );
      self::checkErrno( 0, self::$db -> getError()['errno'] ); 
      self::checkRecordNum( count(self::$data), "bulkInser the record, but part data not the same" );

      $dataOID = array( "_id" => 1024, "test" => "testFlagSDB_FLG_INSERT_RETURN_OID");
      $rc = self::$cl -> bulkInsert( $dataOID, SDB_FLG_INSERT_RETURN_OID );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkRecordNum( count(self::$data) + 1, "bulkInser the same record, check SDB_FLG_INSERT_RETURN_OID" );
      if( $rc['_id'][0] != 1024 )
      {
         throw new Exception( 'check return oid error, exp: 1024, act: ' . $rc['_id'] );
      }

      $testData =  array( "flag" => "test flag = 2");
      self::$cl -> bulkInsert( $testData, 2 );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkRecordNum( count(self::$data) + 2, "test flag = 2" );

      $datas = array();
      $datas[0] = array( "_id" => 1, "test" => "testFlagSDB_FLG_INSERT_RETURN_OID_1" );
      $datas[1] = array( "_id" => 2, "test" => "testFlagSDB_FLG_INSERT_RETURN_OID_2" );
      $datas[2] = array( "_id" => 3, "test" => "testFlagSDB_FLG_INSERT_RETURN_OID_3" );
      $datas[3] = array( "_id" => 4, "test" => "testFlagSDB_FLG_INSERT_RETURN_OID_4" );
      $datas[4] = array( "_id" => 5, "test" => "testFlagSDB_FLG_INSERT_RETURN_OID_5" );
      $rc_recordIds = self::$cl -> bulkInsert( $datas, SDB_FLG_INSERT_RETURN_OID );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      $idArray = $rc_recordIds['_id'];
      if( count($idArray) != count($datas) )
      {
         throw new Exception( 'return record id num error, exp: 5, act: ' . count($idArray) );
      }
      sort( $idArray );
      for( $i = 0; $i < count( $idArray ); $i++ )
      {
          if( $idArray[$i] != $i + 1)
          {
             throw new Exception( 'check record id error:' . $idArray );
          }
      }
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
      echo "\n---Test 16775 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
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

   private static function buildData()
   {
      $datas = array();
      $datas[0] = array( "int" => 1024 );
      $datas[1] = array( "double" => 1024.4096 );
      $datas[2] = array( "string" => "test bulkInsert" );
      $datas[3] = array( "OID" => new SequoiaID( '123456789012345678901234' ) );
      $datas[4] = array( "boolean" => false );
      $datas[5] = array( "date" => new SequoiaDate( '2000-01-01' ) );
      $datas[6] = array( "timestamp" => new SequoiaTimestamp( '2000-01-01-12.30.20.123456' ) );
      $datas[7] = array( "binary" =>  new SequoiaBinary( 'aGVsbG8=', '1' ) );
      $datas[8] = array( "object" => array( "a" => "test" ) );
      $datas[9] = array( "regex" => new SequoiaRegex( 'a', 'i' ) );
      $datas[10] = array( "array" => array( "a", "b", "c") );
      return $datas;
   }

   private static function createUniqueIndex()
   {
      $keyArr = array('int' => 1, 'double' => 1, 'string' => 1, 'OID' => 1, 'boolean' => 1, 'date' => 1, 'timestamp' => 1, 'binary' => 1, 'object' => 1, 'regex' => 1, 'array' => 1);
      self::$cl -> createIndex( $keyArr, 'index16775', true );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
   }

   private static function checkRecordNum( $expRecordNum, $msg = "" )
   {
      $actRecordNum = self::$cl -> count();
      if( $actRecordNum != $expRecordNum )
      {
          throw new Exception( "expect [".$expRecordNum."] but found [".$actRecordNum."]. ".$msg );
      }
   }
}

?>
