cription:      test getIndexes and getIndexInfo
@testlink cases:   seqDB-18029
@input:        test index
@output:     success
@modify list:
        2019-3-15 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class Index18029 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs18029";
   private static $clName = "cl18029";
   private static $indexName1 = "index18029_1";
   private static $indexName2 = "index18029_2";
   private static $indexName3 = "index18029_3";
   private static $db;
   private static $cl;

   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
      
      self::$cl = self::$db -> selectCS(self::$csName) -> selectCL(self::$clName);

      self::$cl -> createIndex( array( "a" => 1), self::$indexName1, false, false);
      $this -> assertEquals( 0, self::$db -> getError()['errno']);

      self::$cl -> createIndex( array( "b" => -1), self::$indexName2, true, false);
      $this -> assertEquals( 0, self::$db -> getError()['errno']);

      self::$cl -> createIndex( array( "c" => -1, "d" => 1), self::$indexName3, true, false);
      $this -> assertEquals( 0, self::$db -> getError()['errno']);
   }

   public function test()
   {
      echo "\n---Begin to test getIndexes.\n";
      
      $cursor = self::$cl -> getIndexes() ;
      if( empty( $cursor ) ) 
      {
         $err = self::$db -> getLastErrorMsg() ;
         throw new Exception("Failed to get indexes, error code: ".$err['errno'] );
      }
      $expNameArr = array();
      $expNameArr[0] = '$id';
      $expNameArr[1] = self::$indexName1;
      $expNameArr[2] = self::$indexName2;
      $expNameArr[3] = self::$indexName3;

      while( $record = $cursor -> next() ) 
      {
         $this -> assertTrue( in_array( $record["IndexDef"]["name"], $expNameArr), $record);
      }

      echo "\n---Begin to test getIndexInfo.\n";

      $indexInfo1 = self::$cl -> getIndexInfo( self::$indexName3 ) ;
      if ( empty( $indexInfo1 ) )
      {
         $err = self::$db -> getLastErrorMsg() ;
         echo "Failed to get index, error code: ".$err['errno'] ;
      }
      $this -> assertEquals( $indexInfo1["IndexDef"]["name"], self::$indexName3 );
      $this -> assertEquals( $indexInfo1["IndexDef"]["key"], array("c" => -1, "d" => 1) );
      $this -> assertEquals( $indexInfo1["IndexDef"]["unique"], true );
      $this -> assertEquals( $indexInfo1["IndexDef"]["enforced"], false );

      echo "\n---Begin to test get not exist index IndexInfo.\n";

      self::$cl -> getIndexInfo( "indexNotExist" );
      $this -> assertEquals( self::$db -> getError()['errno'], -47);
   }

   public function tearDown()
   {
      $err = self::$db -> dropCS( self::$csName );
      if ( $err['errno'] != 0 )
      {
         throw new Exception('failed to drop cs, errno='.$err['errno']);
      }
      echo "\n---End of the test.\n";
      self::$db->close();
   }
}
