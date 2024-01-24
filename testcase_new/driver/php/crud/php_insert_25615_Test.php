/*
 * @Description   : seqDB-25615:php驱动测试insert返回记录数
 * @Author        : yinxiaoxia
 * @CreateTime    : 2022.03.23
 * @LastEditTime  : 2022.03.26
 */
<?php
include_once dirname(__FILE__).'/../global.php';
class TestInsert25615 extends PHPUnit_Framework_TestCase
{
   private static $csName = "cs25615";
   private static $clName = "cl25615";
   private static $csDB;
   private static $clDB;
   private static $insertRecords;
   private static $db;
   private static $InsertedNum = 0;
   private static $duplicatedNums = 0;
   private static $DeletedNum = 0;
   private static $UpdatedNum = 0;
   private static $ModifiedNum = 0;

   public static function setUpBeforeClass()
   {
       $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
       self::$db = new Sequoiadb();
       $err = self::$db -> connect($address, '', '');
       if ( $err['errno'] != 0 )
       {
           throw new Exception("failed to connect db, errno=".$err['errno']);
       }
            
       // create cs
       $err = self::$db -> createCS( self::$csName );
       if ( $err['errno'] != 0 )
       {
           throw new Exception("failed to create cs, errno=".$err['errno']);
       }
      
       // get cs
       $csDB = self::$db -> getCS( self::$csName );
       $err  = self::$db -> getError();
       if ( $err['errno'] != 0 )
       {
           throw new Exception("failed to get cs, errno=".$err['errno']);
       }      
      
       // create cl
       self::$clDB = $csDB -> selectCL( self::$clName );
       $err  = self::$db -> getError();
       if ( $err['errno'] != 0 )
       {
           throw new Exception("failed to create cl, errno=".$err['errno']);
       }
   }

   public function test_Insert()
   {
       // insert
       $recordNum = 5;
       self::$insertRecords = self::insertData(self::$clDB,$recordNum);
       $this->assertEquals(self::$InsertedNum,5);
       $this->assertEquals(self::$duplicatedNums,0);
       // update
       $data = self::$clDB -> update('{ "$set" :{"a":12} }', array( 'b' => array( '$lt' => 3 ) ));
       self::$UpdatedNum = $data["UpdatedNum"];
       self::$ModifiedNum = $data["ModifiedNum"];
       $this->assertEquals(self::$UpdatedNum,2);
       $this->assertEquals(self::$ModifiedNum,2);
       // upsert
       $data = self::$clDB -> upsert('{"$set" :{"b":12}}',array( 'b' => array( '$lte' => 3 ) ));
       self::$UpdatedNum = $data["UpdatedNum"];
       self::$ModifiedNum = $data["ModifiedNum"];
       self::$InsertedNum = $data["InsertedNum"];
       $this->assertEquals(self::$UpdatedNum,3);
       $this->assertEquals(self::$ModifiedNum,3);
       $this->assertEquals(self::$InsertedNum,0);
       // remove
       $data = self::$clDB -> remove(array('a' => array('$lte' => 3)),null);
       self::$DeletedNum = $data["DeletedNum"];
       $this->assertEquals(self::$DeletedNum,1);
   }

   public function insertData($clDB,$recordNum)
   {
       $array = array();
       $flag = SDB_FLG_INSERT_RETURN_OID;
       for ($i=1; $i <= $recordNum; $i++) { 
           $arr = array();
           $arr["a"] = $i;
           $arr["b"] = $i;
           array_push($array,$arr);
           $data = $clDB -> insert("{a:".$i.",b:".$i."}",$flag);
           self::$InsertedNum += $data["InsertedNum"];
           self::$duplicatedNums += $data["DuplicatedNum"];
       }
     return $array;
   }

   public static function tearDownAfterClass()
   {
       self::$db -> dropCS(self::$csName);      
       self::$db->close();
   }
    
}

?>