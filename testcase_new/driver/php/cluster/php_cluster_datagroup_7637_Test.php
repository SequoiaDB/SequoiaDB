/****************************************************
@description:      test datagroup operate,
@testlink cases:   seqDB-7639/7641
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/ReplicaGroupMgr.php';
include_once dirname(__FILE__).'/../global.php';
class dataGroupTest extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $groupMgr;
   
   private static $skipTestCase = false;
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      $err = self::$db -> connect(globalParameter::getHostName().':'. 
                                  globalParameter::getCoordPort()) ;
      if ( $err['errno'] != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return;
      } 
      
      self::$groupMgr = new ReplicaGroupMgr(self::$db);
      if ( self::$groupMgr->getError() != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return;
      } 
      
      if (self::$groupMgr->getDataGroupNum() < 2)
      {
         self::$skipTestCase = true ;
         return;
      } 
   }
   
   public function setUp()
   {
      if( self::$skipTestCase === true )
      {
         $this -> markTestSkipped( "init failed" );
      }
   }
   
   public function testCreate()
   {
      $group = self::$groupMgr->addDataGroup('db'.getmypid());
      $err = self::$groupMgr->getError();
      $this->assertEquals( 0, $err['errno'] ) ;
      $name = $group->getName();
      $this->assertEquals( $name, 'db'.getmypid() ) ;
      
      return $group;
   }
   
   public function testSelectParameter()
   {
      if (mt_rand(0,1) == 0)
      {
         return array('weight'=> 100);
      }
      else
      {
         return json_encode(array('weight'=> 100));
      }
      
   }
   
   /**
    * @depends testCreate
    * @depends testSelectParameter
    */
   public function testAddNode($group, $options)
   {
      $hosts = self::$groupMgr->getAllHostNamesOfDeploy() ;
      $hostName = $hosts[mt_rand(0,count($hosts)-1)] ;
      $port = mt_rand(globalParameter::getSpareportStart(), globalParameter::getSpareportStop()) ;

      $ret = $group->addNode($hostName, $port,globalParameter::getDbPathPrefix()."/".$port, $options);
      $this->assertEquals(0, $ret ) ;
      $node = $group->getNode($hostName.":".$port);
      $err = $node->start();
      $this->assertEquals($err, 0);
      
      $nodedb = $node->connect();
      $this->assertEquals(true, !empty($nodedb));
      $cursor = $nodedb->list(SDB_LIST_CONTEXTS_CURRENT);
      $this->assertEquals(true, !empty($cursor));
      while( $record = $cursor -> next() ) ;
      return $node;
   }
   
   /**
    * @depends testCreate
    * @depends testAddNode
    */
   public function testAddNodeRepeat($group, $node)
   {
      $ret = $group->addNode($node->getHostName(), $node->getServiceName(),globalParameter::getDbPathPrefix().'/'.$node->getServiceName() );
      $this->assertEquals($ret, -145);
   }
   
   /**
    * @depends testCreate
    * @depends testAddNode
    */
   public function testRemoveNode($group, $node)
   { 
      $err = $node->stop();
      $nodedb = $node->connect();
      $this->assertEquals(true, empty($nodedb)); 
      $err = $group->removeNode($node->getHostName(), $node->getServiceName());
      $this->assertEquals($err, -204 ); 
      $tmpNode = $group->getNode($node->getHostName().":".$node->getServiceName());
      $this->assertEquals(true, !empty($tmpNode));  
   }
  
   public static function tearDownAfterClass()
   {
      if ( isset(self::$groupMgr) && self::$groupMgr->getGroupNum() > 1)
      {
         self::$groupMgr->removeGroup('db'.getmypid());
      }
      //$this->assertEquals( 0, $err ) ;
      self::$db->close();
   }
}
?>

