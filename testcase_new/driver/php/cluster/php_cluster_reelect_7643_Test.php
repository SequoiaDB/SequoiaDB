/****************************************************
@description:      test reelect operate
@testlink cases:   seqDB-7643/7642
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/ReplicaGroupMgr.php';
include_once dirname(__FILE__).'/../global.php';
class reelectTest extends PHPUnit_Framework_TestCase
{
   private static $db ;
   private static $groupMgr ;
   private static $groups ;
   private static $skipTestCase ;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb() ;
      $err = self::$db -> connect(globalParameter::getHostName().':'. 
                                  globalParameter::getCoordPort()) ;
       if ( $err['errno'] != 0 )
       {
          echo "Failed to connect database, error code: ".$err['errno'] ;
          self::$skipTestCase = true ;
          return;
       }      
      
      self::$groupMgr = new ReplicaGroupMgr( self::$db ) ;
      self::$groups = self::$groupMgr->getGroups() ;
      if ( count(self::$groups) == 0 )
      {
         $err = self::$groupMgr->getError() ;
         echo "Failed to call listGroup, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
      }
   }
   
   public function setUp()
   {
      if( self::$skipTestCase === true )
      {
         $this -> markTestSkipped( "connect failed" ) ;
      }
   }
  
   private function findNode( $nodes, $node )
   {
      for ( $i = 0; $i < count($nodes); ++$i )
      { 
         $tmp = $nodes[$i];
         if ( $tmp->getName() == $node->getName() )
         {
            echo "leave findNode\n";
            return true;
         }
      }
      return false;
   }

   public function testSelectParameter()
   {
      $options = array('Seconds'=> 60);
      $selector = mt_rand( 0, 2 );
      if ( $selector == 0 )
      {
         return $options;
      }
      else if ( $selector == 1 )
      {
         return json_encode( $options );
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
   public function testReelect($options)
   {
      if (empty( self::$groups )) return;
      for ($i =0; $i < count( self::$groups ); ++$i)
      {
         $group = self::$groups[$i];
         $groupName = $group->getName();
         
         if ( $group->getNodeNum() > 1 &&
             ( $groupName != "SYSCoord" &&
               $groupName != "SYSCatalogGroup" &&
               $groupName != "SYSSpare" ))
         {
            break;
         }
      }
       
      $nodes = $group->getNodes();
      $this->assertEquals( $group->getNodeNum(), count($nodes) ) ;
      
      $node = $group->getMaster();
      $this->assertEquals(true, !empty($node)) ;
      $this->assertEquals(true, $this->findNode($nodes, $node)) ;
      
      $snode = $group->getSlave();
      $this->assertEquals(true, !empty($snode)) ;
      $this->assertEquals(true, $this->findNode($nodes, $snode)) ;
      
      $err = $group->reelect($options);
      $this->assertEquals( 0, $err['errno'] ) ;
      
      $node = $group->getMaster();
      $this->assertEquals(true, !empty($node)) ;
      $this->assertEquals(true, $this->findNode($nodes, $node)) ;
      
   }
   
   public static function tearDownAfterClass()
   {
      $err = self::$db->close();
   }
}
?>
