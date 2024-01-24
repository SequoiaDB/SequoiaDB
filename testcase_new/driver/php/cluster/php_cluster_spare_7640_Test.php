/****************************************************
@description:      test spare group operate
@testlink cases:   seqDB-7639
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
 
 include_once dirname(__FILE__).'/../commlib/ReplicaGroupMgr.php';
 include_once dirname(__FILE__).'/../global.php';

class spareRGTest extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $groupMgr;
   private static $group ;
   private static $node;
   private static $skipTestCase = false;
   private static $sign = false;

   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      $err = self::$db->connect(globalParameter::getHostName().":". 
                                globalParameter::getCoordPort()) ;
      if ( $err['errno'] != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno']. "\n" ;
         self::$skipTestCase = true ;
         return;
      } 

      self::$db->setSessionAttr(array('PreferedInstance' => 'm' )) ; 
      self::$groupMgr = new ReplicaGroupMgr(self::$db);
      if ( self::$groupMgr->getError()['errno'] != 0 )
      {
         echo "Failed to listGroup, error code: ".$err['errno'] ;
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
   
   
   public function testSelectParameter()
   {
      $option = array('KeepData'=> true);
      if (mt_rand(0,1) == 0)
      {
         return $option;
      }
      else
      {
         return json_encode($option);
      }
      
   }
   
   private function findNode($nodes, $node)
   {
      for ($i = 0; $i < count($nodes); ++$i)
      { 
         $tmp = $nodes[$i];
         if ($tmp->getName() == $node->getName())
         {
            return true;
         }
      }
      
      return false;
   }
   
   /**
    * @depends testSelectParameter
    *
    */
   public function testDetachNode($options)
   {
      self::$group = self::$groupMgr->addDataGroup('SYSSpare') ;
      $err = self::$groupMgr->getError() ;
      $this->assertEquals($err, 0) ;
      $hosts = self::$groupMgr->getAllHostNamesOfDeploy() ;
   
      $hostName = $hosts[mt_rand(0, count($hosts) -1 )] ;
      $port = mt_rand(globalParameter::getSpareportStart(), globalParameter::getSpareportStop()) ;
      $err = self::$group->addNode($hostName, $port, globalParameter::getDbPathPrefix().'/'.$port);
      $this->assertEquals($err, 0) ;
      self::$node = self::$group->getNode($hostName.":".$port );
      $this->assertEquals(true, !empty(self::$node)) ;
      
      $err = self::$node->start();
      $this->assertEquals($err, 0) ;

      $err = self::$group->detachNode(self::$node);
      $this->assertEquals($err, -6) ;

      $err = self::$group->detachNode(self::$node, null);
      $this->assertEquals($err, -6) ;
      
      $err = self::$group->detachNode(self::$node, $options);
      $this->assertEquals($err, 0) ;
      
      sleep(5);
      $tmpNode = self::$group->getNode(self::$node->getName()) ;
      $this->assertEquals(false, isset($tmpNode));
      //$this->assertEquals($this->findNode($nodes, self::$node), false); 
      
    
      return self::$node;
      
   }
   
   /**
    * @depends testSelectParameter
    * @depends testDetachNode 
    */
   public function testAttachNode($options, $node)
   {
      
      $groups = self::$groupMgr->getDataGroups();
      
      $pos = mt_rand(0,count($groups)-1);
      
      self::$group = $groups[$pos];
      $nodeNum = self::$group->getNodeNum();
      
      $err = self::$group->attachNode($node, null);
      $this->assertEquals($err, -6);

      $err = self::$group->attachNode($node);
      $this->assertEquals($err, -6);

      $err = self::$group->attachNode($node, $options);
      // 报错-13，无法获取日志，错误无法定位，添加条件对节点进行保留
      if ($err != 0 )
      {
         self::$sign = true;
      }

      $this->assertEquals($err, 0) ;
      $this->assertEquals($nodeNum+1, self::$group->getNodeNum());
      $node = self::$group->getNode($node->getName());
      $this->assertEquals(isset($node), true);
      
   }

   public static function tearDownAfterClass()
   {
      if (isset(self::$groupMgr) == true && !self::$sign) 
      {
         $err = self::$groupMgr->removeGroup("SYSSpare");
      }

      if (isset(self::$group) == true && isset(self::$node) == true  && !self::$sign)
      {
         $err = self::$group->removeNode(self::$node->getHostName(), self::$node->getServiceName()) ;
      }
      //$err = self::$group->drop();
      $err = self::$db->close();
   }
}
?>

