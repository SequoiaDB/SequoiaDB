/****************************************************
@description:     test configure update/delete/snapshot
@testlink cases:  seqDB-15243
@modify list:
      2018-06-07  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/ReplicaGroupMgr.php';

class TestConf15243 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $position;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      if( !self::isStandAlone( self::$db ) )
      {
         $rgMgr = new ReplicaGroupMgr( self::$db );
         $dataGroup = $rgMgr -> getDataGroups()[0];
         $dataNode = $dataGroup -> getMaster(); 
         $hostName = $dataNode -> getHostName();
         $svcName = $dataNode -> getServiceName();
         self::$position = array( "HostName" => $hostName, "svcname" => $svcName );
      }
      else
      {
         self::$position = array( "Global" => false );
      }
   }
   
   function test()
   {
      $defValue = self::getWeightFromSnap();
      $configs = array( "weight" => 20 );
      $options = self::$position;

      self::$db -> updateConfig( $configs, $options );
      $updateValue = self::getWeightFromSnap();
      self::checkEquals( $configs, $updateValue, "update configure not work" );

      self::$db -> deleteConfig( $configs, $options );
      $deleteValue = self::getWeightFromSnap();
      self::checkEquals( $defValue, $deleteValue, "delete configure not work" );
   }
   
   public static function tearDownAfterClass()
   {
      self::$db -> close();
   }

   private function isStandAlone( $db )
   {
      $db -> listGroup();
      $errno = $db -> getError() ['errno'];
      if( $errno == -159 )
         return true;
      else if( $errno == 0 )
         return false;
      else
         throw new Exception("unexpected sdb error: ".$errno);
   }

   // assert message has no line number. that's bad. so I use Exception instead.
   public function checkEquals( $expVal, $actVal, $msg = "" )
   {
      if( $expVal != $actVal ) 
      {
         throw new Exception( "expect [".$expVal."] but found [".$actVal."]. ".$msg );
      }
   }

   public function getWeightFromSnap()
   {
      $cond = self::$position;
      $select = array( "weight" => 1 );
      $cursor = self::$db -> snapshot( SDB_SNAP_CONFIGS, $cond, $select );
      self::checkEquals( 0, self::$db -> getError()['errno'] );
      $value = $cursor -> next();
      self::checkEquals( 0, self::$db -> getError()['errno'] );
      return $value;
   }
}
?>
