/****************************************************
@description:      seqDB-20158:listLobPieces查询条件测试
@modify list:
        2019-10-29 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class lob20118 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $csName = 'cs20158';
   private static $mainCLName = 'maincl20158';
   private static $subCLName = 'subcl20158';
   
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
      
      $content = array('hello', 'hellohello', 'hellohellohello', 'hellohellohellohello', 'hellohellohellohellohello');
      for($i = 0; $i < 5; $i++)
      {
         $lobId = $mainCL -> createLobID();
         $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
         $lob = $mainCL -> openLob( $lobId, SDB_LOB_CREATEONLY );
         $this -> assertEquals( 0, self::$db -> getError()['errno'] );
         $lob -> write( $content[$i] );
         $lob -> close();
      }
      
      $cursor = $mainCL -> listLobPieces(array('Sequence' => array( '$et' => 0)),
                               array('Sequence' => array('$include' => 1)),
                               array('Sequence' => 1),
                               array('""' => 'ListPieces'), 1, 2) ;
      $records = array();
      while($record = $cursor -> next())
      {
         array_push($records, $record);
      }
      $this -> assertEquals( 2, count($records));
      $this -> assertEquals( 0, $records[0]['Sequence']);
      $this -> assertEquals( 0, $records[1]['Sequence']);
   }
   
   public function tearDown()
   {
      echo "\n---check lob subcl complete.\n";
      self::$db -> dropCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      self::$db -> close();
   }
}