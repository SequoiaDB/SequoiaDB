/****************************************************
@description:     test snapshot access plan
@testlink cases:  seqDB-14513
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestSnapAccessPlan14513 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = "cs14513";
   private static $clName = "cl14513";
   private static $cl;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$cl = self::createCSCL( self::$db, self::$csName, self::$clName );
      self::insertData( self::$cl, 1000 );
   }
   
   function test()
   {
      $cond = '{ "no": { "$lt": 100 } }';
      self::findWithCond( self::$cl, $cond );
      $cond = '{ "no": { "$gte": 500 } }';
      self::findWithCond( self::$cl, $cond );
      $cond = '{ "$and": [{ "no": { "$lt": 400} }, { "no": { "$gte": 200 } }] }';
      self::findWithCond( self::$cl, $cond );

      $cond = '{ Collection: "'.self::$csName.'.'.self::$clName.'" }';
      $select = '{ "MinTimeSpentQuery.ReturnNum": { $include: 1 } }';
      $order = '{ "MinTimeSpentQuery.ReturnNum": 1 }';
      $cursor = self::$db -> snapshot( SDB_SNAP_ACCESSPLANS, $cond, $select, $order );
      self::checkErrno( 0, self::$db -> getError()['errno'] );

      $actRecs = array();
      while( $record = $cursor -> next() )
         array_push( $actRecs, $record );
      if( is_object( $actRecs[0]['MinTimeSpentQuery']['ReturnNum'] ) )
      {
         $expRecs = array
         (
            array( 'MinTimeSpentQuery' => array( 'ReturnNum' => new SequoiaINT64( '100' ) ) ),
            array( 'MinTimeSpentQuery' => array( 'ReturnNum' => new SequoiaINT64( '200' ) ) ),
            array( 'MinTimeSpentQuery' => array( 'ReturnNum' => new SequoiaINT64( '500' ) ) )
         );
      }
      else
      {
         $expRecs = array
         (
            array( 'MinTimeSpentQuery' => array( 'ReturnNum' => 100 ) ),
            array( 'MinTimeSpentQuery' => array( 'ReturnNum' => 200 ) ),
            array( 'MinTimeSpentQuery' => array( 'ReturnNum' => 500 ) )
         );
      }
      
      self::checkArrayEqual( $expRecs, $actRecs );
   }
   
   public static function tearDownAfterClass()
   {
      self::$db -> dropCS( self::$csName );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      self::$db -> close();
   }

   // assert message has no line number. that's bad. so I use Exception instead.
   private static function checkErrno( $expErrno, $actErrno, $msg = "" )
   {
      if( $expErrno != $actErrno ) 
      {
         throw new Exception( "expect [".$expErrno."] but found [".$actErrno."]. ".$msg );
      }
   }

   private static function createCSCL( $db, $csName, $clName )
   {
      $db -> createCS( $csName );
      self::checkErrno( 0, $db -> getError()['errno'] );
      $cs = $db -> getCS( $csName );
      self::checkErrno( 0, $db -> getError()['errno'] );
      $cs -> createCL( $clName );
      self::checkErrno( 0, $db -> getError()['errno'] );
      $cl = $cs -> getCL( $clName );
      self::checkErrno( 0, $db -> getError()['errno'] );
      return $cl;
   }

   private static function insertData( $cl, $recNum )
   {
      $recs = array();
      for( $i = 0; $i < $recNum; $i++ )
      {
         array_push( $recs, array( 'no' => $i ) );
      }
      $err = $cl -> bulkInsert( $recs, 0 );
      self::checkErrno( 0, $err ['errno'] );
   }

   private static function findWithCond( $cl, $cond )
   {
      $cursor = $cl -> find( $cond );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      while( $record = $cursor -> next() ) {}
   }

   private static function checkArrayEqual( $expRecs, $actRecs )
   {
      if( $expRecs != $actRecs )
      {
         var_dump( $expRecs );
         var_dump( $actRecs );
         throw new Exception( "wrong result. array is different!" );
      }
   }
}
?>
