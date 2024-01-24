/****************************************************
@description:      check sdb.list() option
@testlink cases:   seqDB-20008
@modify list:
        2019-10-11 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class List20008 extends PHPUnit_Framework_TestCase
{
   private static $db;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      self::checkErrno( 0, self::$db -> getError()['errno'] );                     
   }
   
   function test()
   {
      echo "\n---Begin to check list.\n";

      $userArr = array( "admin1", "admin2", "admin3", "admin4", "admin5");

      foreach( $userArr as $i => $user )
      {
         self::$db -> createUser( $user, $user );
      }

      //counts the number of nodes in the current cluster
      $skipNum = count( $userArr ) - 1;
      $returnNum = 1;
      $listCur = self::$db -> list( SDB_LIST_USERS, null, null, null, array("" => "test"), $skipNum, -1 );
      if( empty( $listCur ) ) 
      {
	 foreach( $userArr as $i => $user )
         {
            self::$db -> removeUser( $user, $user );
         }
         throw new Exception('get list SDB_LIST_USERS error, is empty');
      }
      $times = 0;
      while( $record = $listCur -> next())
      {
         $times++;
      }
      if( $times != $returnNum)
      {
         foreach( $userArr as $i => $user )
	 {
	    self::$db -> removeUser( $user, $user );
	 }
	 throw new Exception( 'check list record num error, exp: '. $returnNum .', act: ' . $times );
      }

      $listCur = self::$db -> list( SDB_LIST_USERS, null, null, null, array("" => "test"), 0, $returnNum );
      if( empty( $listCur ) )
      {
	 foreach( $userArr as $i => $user )
	 {
	    self::$db -> removeUser( $user, $user );
         }
         throw new Exception('get list SDB_LIST_USERS error, is empty');
      }
      $times = 0;
      while( $record = $listCur -> next())
      {
         $times++;
      }
      if( $times != $returnNum)
      {
	 foreach( $userArr as $i => $user )
         {
	    self::$db -> removeUser( $user, $user );
	 }
         throw new Exception( 'check list record num error, exp: '. $returnNum .', act: ' . $times );
      }
      foreach( $userArr as $i => $user )
      {
         self::$db -> removeUser( $user, $user );
      }

      $taskCur = self::$db -> list( SDB_LIST_SVCTASKS );
      if( empty( $taskCur ) )
      {
	 throw new Exception('get list SDB_LIST_SVCTASKS error, is empty');
      }
      if( $record = $taskCur -> next())
      {
	 if(!isset( $record["NodeName"]))throw new Exception('list SDB_LIST_SVCTASKS content error, no NodeName');
         if(!isset( $record["TaskID"]))throw new Exception('list SDB_LIST_SVCTASKS content error, no TaskID');
	 if(!isset( $record["TaskName"]))throw new Exception('list SDB_LIST_SVCTASKS content error, no TaskName');
      }
      else
      {
	 throw new Exception('get list SDB_LIST_SVCTASKS error, nothing is returned');
      }

      $taskSnapCur = self::$db -> snapshot( SDB_SNAP_SVCTASKS );
      if( empty( $taskSnapCur ) )
      {
         throw new Exception('get list SDB_SNAP_SVCTASKS error, is empty');
      }
      if( $record = $taskSnapCur -> next())
      {
         if(!isset( $record["TaskName"]))throw new Exception('list SDB_SNAP_SVCTASKS content error, no TaskName');
         if(!isset( $record["TaskID"]))throw new Exception('list SDB_SNAP_SVCTASKS content error, no TaskID');
         if(!isset( $record["Time"]))throw new Exception('list SDB_SNAP_SVCTASKS content error, no Time');
      }
      else
      {
         throw new Exception('get list SDB_SNAP_SVCTASKS error, nothing is returned');
      }

      self::$db -> selectCS("cs20008") -> selectCL("cl20008") -> createAutoIncrement( array( 'Field' => 'a', 'MaxValue' => 2000 ) );
      self::checkErrno( 0, self::$db -> getError()['errno'] );
      $sequenceCur = self::$db -> list( SDB_LIST_SEQUENCES );
      if( empty( $sequenceCur ) )
      {
         throw new Exception('get list SDB_LIST_SEQUENCES error, is empty');
      }
      if( $record = $sequenceCur -> next())
      {
         if(!isset( $record["Name"]))throw new Exception('list SDB_LIST_SEQUENCES content error, no Name');
      }
      else
      {
	 throw new Exception('get list SDB_LIST_SEQUENCES error, nothing is returned');
      }
      self::$db -> dropCS("cs20008");
      self::checkErrno( 0, self::$db -> getError()['errno'] );
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---check list complete.\n";
      self::$db->close();
   }
   
   private static function checkErrno( $expErrno, $actErrno, $msg = '' )
   {
      if( $expErrno != $actErrno ) 
      {
         throw new Exception( 'expect ['.$expErrno.'] but found ['.$actErrno.']. '.$msg );
      }
   }
   
}
?>
