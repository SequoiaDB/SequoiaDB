/****************************************************
@description:      主子表创建/读取/删除lob
@modify list:
        2019-10-29 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class lob20117 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = 'cs20117';
   private static $mainCLName = 'maincl20117';
   private static $subCLName = 'subcl20117';
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   }
   
   function test()
   {
      echo "\n---Begin to test lob subcl.\n";
      
      $cs = self::$db -> selectCS(self::$csName);
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $mainCL = $cs -> selectCL(self::$mainCLName, array('IsMainCL' => true, 
                                                         'ShardingKey' => array( 'date' => 1 ),
                                                         'ShardingType' => 'range', 
                                                         'LobShardingKeyFormat' => 'YYYYMMDD'));
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $cs -> createCL(self::$subCLName);
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $mainCL -> attachCL(self::$csName . '.' . self::$subCLName, 
                          array( 'LowBound' => array( 'date' => new SequoiaMinKey() ), 
                          'UpBound' => array( 'date' => new SequoiaMaxKey() ) ));
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $lobId = $mainCL -> createLobID("0000-01-01-00.00.00");
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $lob = $mainCL -> openLob( $lobId, SDB_LOB_CREATEONLY );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      $lob -> write( 'hello' );
      $lob -> close();
      
      $lobId2 = $mainCL -> createLobID("9999-12-31-00.00.00");
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $lob2 = $mainCL -> openLob( $lobId2, SDB_LOB_CREATEONLY );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      $lob2 -> write( 'hello' );
      $lob2 -> close();
      
      $lob = $mainCL -> openLob( $lobId, SDB_LOB_READ );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $buffer = $lob -> read( 5 ) ;
      $this -> assertEquals( 'hello', $buffer );
      $lob -> close();
      
      $mainCL -> removeLob( $lobId );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      $lob = $mainCL -> openLob( $lobId, SDB_LOB_READ );
      $this -> assertEquals( -4, self::$db -> getError()['errno'] );
   }
   
   public function tearDown()
   {
      echo "\n---check lob subcl complete.\n";
      self::$db -> dropCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$db -> close();
   }
}