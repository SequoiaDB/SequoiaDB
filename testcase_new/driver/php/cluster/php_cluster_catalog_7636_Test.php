/****************************************************
@description:      test catalog group 
@testlink cases:   seqDB-7636
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/ReplicaGroupMgr.php';
include_once dirname(__FILE__).'/../global.php';

class cataLogGroupTest extends PHPUnit_Framework_TestCase 
{
   private static $db;
   private static $groupMgr;
   private static $skipTestCase = false;
   
   public static function setUpBeforeClass()
   {
      $coordHostName = globalParameter::getHostName();
      $coordPort = globalParameter::getHostName();
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
      $err = self::$groupMgr->getError();
      if ( $err['errno'] != 0 )
      {
         echo "Failed to listGroup, error code: ".$err['errno'] ;
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
      $random = mt_rand(0,2);
      if ( $random == 0)
      {
         return array('auth'=> true);
      }
      else if ($random == 1)
      {
         return json_encode(array('auth'=> true));
      }
      else
      {
         return NULL;
      }
      
   }
   
   /**
    * @depends testSelectParameter
    *
    */
   public function testCreateByfullParameters($options)
   {
      $hosts = self::$groupMgr->getAllHostNamesOfDeploy() ;
   
      $hostName = $hosts[mt_rand(0,count($hosts)-1)] ;
      $port = mt_rand(globalParameter::getSpareportStart(), globalParameter::getSpareportStop()) ;
      $group = self::$groupMgr->addCatalogGroup($hostName, $port, globalParameter::getDbPathPrefix().'/'.$port,$options);
    
      $this->assertEquals( 0, self::$groupMgr->getError() ) ;
      if ( self::$groupMgr->getGroupNum() > 1) return;
      
      $node = $group->getNode($hostName, $port);
      $node = $nodes[0];
      $this->assertEquals( $hostName,  $node->getHostName()) ;
      $this->assertEquals( $port,  $node->getServiceName()) ;
      
      $totalSleepTime = 20;
      $alreadySleepTime = 0;
      $nodedb = $node->connect();
      while(empty($nodedb)){
         sleep(1);
         $alreadySleepTime +=1;
         $ret = $node->connect();
         if ($alreadySleepTime > $totalSleepTime) 
            break;
      }
      $this->assertEquals( true,  !empty($nodedb)) ;
      $this->assertEquals(true,  $group->isCataLog()) ;
      $err = $group->stop();
      $this->assertEquals( $err,  0) ;
      
      $nodedb = $node->connect();
      $this->assertEquals( true, empty($nodedb)) ;
      
      $err = $group->start();
      $this->assertEquals( $err,  0) ;
      
      $nodedb = $node->connect();
      $this->assertEquals( true,  !empty($nodedb)) ;
       
   }
   
   protected function tearDown()
   {
      if (isset(self::$groupMgr) == true && self::$groupMgr->getGroupNum() == 1)
      {
         self::$groupMgr->removeGroup("SYSCatalogGroup");
      }
      $err = self::$db->close();
      $this->assertEquals( 0, $err['errno'] ) ;
   }
}
?>

