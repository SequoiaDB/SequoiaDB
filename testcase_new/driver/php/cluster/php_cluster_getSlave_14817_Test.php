/****************************************************
@description:     test getSlave with option
@testlink cases:  seqDB-14817
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class TestGetSlave14817 extends PHPUnit_Framework_TestCase
{
   private static $db;
   
   public static function setUpBeforeClass()
   {
      echo "\n---Begin to run TestGetSlave14817.";
      self::$db = new Sequoiadb();
      $err = self::$db -> connect(globalParameter::getHostName().':'. 
                                  globalParameter::getCoordPort()) ;
      self::assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   function test()
   {
      if( $this -> isStandAlone( self::$db ) )
         return ;
      $group = $this -> getDataGroup( self::$db );
      $this -> assertEquals( 0, self::$db -> getError() ['errno'] );

      $node = $group -> getSlave('a');
      $this -> assertEquals( -6, self::$db -> getError() ['errno'] );
      $node = $group -> getSlave([2, 2, 'a', 4]);
      $this -> assertEquals( -6, self::$db -> getError() ['errno'] );
      $node = $group -> getSlave([8]);
      $this -> assertEquals( -6, self::$db -> getError() ['errno'] );
      $node = $group -> getSlave(1);
      $this -> assertEquals( -6, self::$db -> getError() ['errno'] );
   }
   
   public static function tearDownAfterClass()
   {
      self::$db -> close();
      echo "\n---End to run TestGetSlave14817.";
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

   private function getDataGroup( $db )
   {
      $cursor = $db -> listGroup();
      $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
      while( $record = $cursor -> next() )
      {
         $groupName = $record['GroupName'] ;
         if( $groupName == 'SYSCoord' || $groupName == 'SYSCatalogGroup' )
            continue;
         $group = $db -> getGroup( $groupName );
         $this -> assertEquals( 0, self::$db -> getError() ['errno'] );
         return $group;
      }
      throw new Exception("no any data group, stop test");
   }
}
?>
