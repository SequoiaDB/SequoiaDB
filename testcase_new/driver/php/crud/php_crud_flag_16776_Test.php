/****************************************************
@description:     test timestamp increment
@testlink cases:   seqDB-16776
@modify list:
        2018-11-19 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestCURDFlag16776 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs16776";
   private static $clName = "cl16776";
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
      
      self::createUniqueIndex();
   }

   function test()
   {
      echo "\n---Begin to test insert.\n";
      
      //bulkInsert not exist record set flag=0
      $data1 = array( '_id' => 1, 'test' => "testflag0");
      self::$cl -> insert( $data1, 0 );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkRecordNum( 1, array('test' => "testflag0"),  "first insert record" );

      //bulkInsert already exist record set flag=0
      $rc = self::$cl -> insert( $data1, 0 );
      self::checkErrno( -38, self::$db -> getError()['errno'] );
      self::checkRecordNum( 1, array('test' => "testflag0"), "insert the same record" );
      
      $rc = self::$cl -> insert( $data1, SDB_FLG_INSERT_CONTONDUP );
      self::checkErrno( 0, self::$db -> getError()['errno'] ); 
      self::checkRecordNum( 1, array('test' => "testflag0"),  "inser the record, flag = SDB_FLG_INSERT_CONTONDUP" );

      $rc = self::$cl -> insert( array( 'test' => 'recordNotExist' ), SDB_FLG_INSERT_CONTONDUP );
      self::checkErrno( 0, self::$db -> getError()['errno'] ); 
      self::checkRecordNum( 1, array('test' => 'recordNotExist' ),  "inser the record, flag = SDB_FLG_INSERT_CONTONDUP" );

      $data2 = array( '_id' => 2, 'test' => "testflagSDB_FLG_INSERT_RETURN_OID");
      $rc = self::$cl -> insert( $data2, SDB_FLG_INSERT_RETURN_OID );
      if($rc['_id'] != 2)
      {
         throw new Exception( "expect _id = 2  but found _id = ". $rc['_id'] . " set flag = SDB_FLG_INSERT_RETURN_OID");
      }
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkRecordNum( 1, array('test' => "testflagSDB_FLG_INSERT_RETURN_OID"),  "bulkInser the same record, check SDB_FLG_INSERT_RETURN_OID" );

      $testData =  array( "flag" => "testflag=2");
      self::$cl -> insert( $testData, 2 );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::checkRecordNum( 1, array( "flag" => "testflag=2"), "test flag = 2" );
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
      echo "\n---Test 16776 spend time: " . ( self::$endTime - self::$beginTime ) . " seconds.\n";
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

   private static function createUniqueIndex()
   {
      $index = array( "test" => 1);
      self::$cl -> createIndex( $index, 'index16776', true );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
   }

   private static function checkRecordNum( $expRecordNum, $select, $msg = "" )
   {
      $actRecordNum = self::$cl -> count( $select );
      if( $actRecordNum != $expRecordNum )
      {
          throw new Exception( "expect [".$expRecordNum."] but found [".$actRecordNum."]. ".$msg );
      }
   }
}

?>
