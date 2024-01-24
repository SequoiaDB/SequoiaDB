/****************************************************
@description:     test getSlave with option
@testlink cases:  seqDB-14814
                  seqDB-14815
                  seqDB-14816
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestGetSlave14814 extends PHPUnit_Framework_TestCase
{
   private static $db;
   
   public static function setUpBeforeClass()
   {
      echo "\n---Begin to run TestGetSlave14814.";
      self::$db = new Sequoiadb();
      $err = self::$db -> connect(globalParameter::getHostName().':'. 
                                  globalParameter::getCoordPort()) ;
      if( $err['errno'] != 0 )
      {
         throw new Exception( "fail to connect db, error code: ".$err['errno'] );
      }
   }
   
   function testGetSlave()
   {
      if( $this -> isStandAlone( self::$db ) )
         return ;
      $group = $this -> getMultiNodeGroup( self::$db );

      // seqDB-14814 getSlave() without position
      $hasDiffNode = false;
      $firstNodeName = '';
      for( $i = 0; $i < 20; $i++ )
      {
         $node = $group -> getSlave();
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
         $nodeName = $node -> getName();
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );

         $this -> assertTrue( $this -> isSlave( $group, $nodeName ) );
         if( $i == 0 ) 
            $firstNodeName = $nodeName;
         else if( $nodeName != $firstNodeName )
            $hasDiffNode = true;
      }
      if( !$hasDiffNode ) {
         throw new Exception( "getSlave() is not equal probability" );
      }
      // seqDB-14815 getSlave() with one position
      $hasDiffNode = false;
      for( $i = 0; $i < 5; $i++ )
      {
         $node = $group -> getSlave([1]);
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
         $nodeName = $node -> getName();
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );

         if( $i == 0 ) 
            $firstNodeName = $nodeName;
         else if( $nodeName != $firstNodeName )
            $hasDiffNode = true;
      }
      if( $hasDiffNode ) {
         throw new Exception( "getSlave()'s position not work" );
      }
      
      // seqDB-14816 getSlave() without multi-position
      $hasDiffNode = false;
      $firstNodeName = '';
      for( $i = 0; $i < 20; $i++ )
      {
         $node = $group -> getSlave([1, 2, 3]);
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
         $nodeName = $node -> getName();
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );

         $this -> assertTrue( $this -> isSlave( $group, $nodeName ) );
         if( $i == 0 ) 
            $firstNodeName = $nodeName;
         else if( $nodeName != $firstNodeName )
            $hasDiffNode = true;
      }
      if( !$hasDiffNode ) {
         throw new Exception( "getSlave() is not equal probability" );
      }
   }
   
   public static function tearDownAfterClass()
   {
      self::$db -> close();
      echo "\n---End to run TestGetSlave14814.";
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

   private function getMultiNodeGroup( $db )
   {
      $cursor = $db -> listGroup();
      $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
      while( $record = $cursor -> next() )
      {
         $groupName = $record['GroupName'] ;
         if( $groupName == 'SYSCoord' )
            continue;
         $group = $db -> getGroup( $groupName );
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
         $nodeNum = $this -> getNodeNum( $group );
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
         if( $nodeNum >= 3 )
            return $group;
      }
      throw new Exception("no any data group has 3 nodes, stop test");
   }

   private function getNodeNum( $group )
   {
      $detail = $group -> getDetail();
      $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
      $nodeNum = count( $detail['Group'] );
      return $nodeNum;
   }

   private function isSlave( $group, $nodeName )
   {
      $masterNode = $group -> getMaster();
      $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
      $masterName = $masterNode -> getName();
      $this -> assertEquals( 0, self::$db -> getError() ['errno'] );

      return ($nodeName != $masterName);
   }
}
?>
