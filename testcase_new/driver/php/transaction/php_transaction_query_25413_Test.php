 /
 * @Description   : seqDB-25413:事务中指定SDB_FLG_QUERY_FOR_SHARE查询
 * @Description   : seqDB-25481:不开启事务使用SDB_FLG_QUERY_FOR_SHARE查询数据
 * @Author        : yinxiaoxia
 * @CreateTime    : 2022.02.25
 */
<?php
include_once dirname(__FILE__).'/../global.php';
class listTransactionTest25413 extends PHPUnit_Framework_TestCase
{
   protected static $db; 
   protected static $clDB;
   protected static $csDB;
   protected static $csID;
   protected static $clID;
   protected static $extentID;
   protected static $offset;
   protected static $mode;
   protected static $count;
   protected static $insertRecods;
   protected static $LOCK_IS = "IS";
   protected static $LOCK_S = "S";
   protected static $csName = "cs_25413";
   protected static $clName = "cl_25413";

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
      // 插入数据
      $recordNum = 20;
      self::$insertRecods = self::insertData(self::$clDB,$recordNum);
      // 创建索引
      self::$clDB -> createIndex(array('a' => 1),"idx_25413");
   }

   public function test_QueryTrans()
   {
      // 修改会话属性
      self::$db -> setSessionAttr('{"TransMaxLockNum":10}');
      self::$db -> setSessionAttr('{"TransIsolation":1}');
   
      // 开启事务查询10条数据
      self::$db -> transactionBegin();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      $hint = '{"":"idx_25413"}';
      $cursor = self::$clDB -> find(NULL,NULL,NULL,$hint,0,10,0);
      while($cursor -> getNext()){
         var_dump( $cursor );
      }
   
      // 查看当前事务快照
      $cursor = self::$db -> snapshot(SDB_SNAP_TRANSACTIONS_CURRENT);
   
      // 检验没有记录锁为S锁
      $lockCount = self::getCLLockCount(self::$db,self::$LOCK_S);
      $this-> assertEquals($lockCount,0);
   
      // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询10条数据
      $cursor = self::$clDB -> find(NULL,NULL,NULL,$hint,0,10,SDB_FLG_QUERY_FOR_SHARE);
      while($cursor -> getNext()){
         var_dump( $cursor );
      }
   
      // 校验记录锁中S锁数量为10，集合锁为IS，没有发生锁升级
      $lockCount = self::getCLLockCount(self::$db,self::$LOCK_S);
      $this -> assertEquals($lockCount,10);
      self::checkIsLockEscalated(self::$db,false);
      self::checkCLLockType(self::$db,self::$LOCK_IS);
   
      // 不指定flags查询后10条数据
      $cursor = self::$clDB -> find(NULL,NULL,NULL,$hint,10,10,0);
      while($cursor -> getNext()){
         var_dump( $cursor );
      }
   
      // 记录锁数量不变，集合锁不变，没有发生锁升级
      $lockCount = self::getCLLockCount(self::$db,self::$LOCK_S);
      $this -> assertEquals($lockCount,10);
      self::checkIsLockEscalated(self::$db,false);
      self::checkCLLockType(self::$db,self::$LOCK_IS);
   
      // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询后10条数据
      $cursor = self::$clDB -> find(NULL,NULL,NULL,$hint,10,10,SDB_FLG_QUERY_FOR_SHARE);
      while($cursor -> getNext()){
         var_dump( $cursor );
      }
   
      // 发生锁升级，集合锁为S锁
      self::checkIsLockEscalated(self::$db, true );
      self::checkCLLockType(self::$db, self::$LOCK_S );
   
      // 事务中指定flags为SDB_FLG_QUERY_FOR_SHARE读取数据并校验
      $cursor = self::$clDB -> find(NULL,NULL,NULL,NULL,0,-1,SDB_FLG_QUERY_FOR_SHARE);
      self::checkRecords( self::$insertRecods, $cursor );
   
      // 提交事务
      self::$db -> transactionCommit();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
   
      // 不开启事务使用flags为SDB_FLG_QUERY_FOR_SHARE读取数据并校验
      $cursor = self::$clDB -> find(NULL,NULL,NULL,NULL,0,-1,SDB_FLG_QUERY_FOR_SHARE); 
      self::checkRecords( self::$insertRecods, $cursor );
   
   }

   public function getErrno()
   {
      $err = self::$db->getError();
      return $err['errno'];
   }

   public function checkIsLockEscalated($db,$isLockEscalated)
   {
      $cursor = $db -> getSnapShot(SDB_SNAP_TRANSACTIONS_CURRENT,NULL,NULL,NULL);
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to exec snapshot. Errno: ". $errno ."\n";
      }
      $record = $cursor -> next();
      $this -> assertEquals(true,!empty($record));
      $actLockEscalated = $record['IsLockEscalated'];
      $this -> assertEquals($actLockEscalated,$isLockEscalated);
   }

   public function getCLLockCount($db,$lockType)
   {
      $cursor = $db -> getSnapshot(SDB_SNAP_TRANSACTIONS_CURRENT,NULL,NULL,NULL);
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to exec snapshot. Errno: ". $errno ."\n";
      }
      $lockCount = 0;
      $record = $cursor -> next();
      $this -> assertEquals(true, !empty($record));
      $lockList = $record['GotLocks'];
      $arrLen = count($lockList);
      for ($i=0; $i <$arrLen ; $i++) { 
      $csID = $lockList[$i]['CSID'];
      $clID = $lockList[$i]['CLID'];
      $extentID = $lockList[$i]['ExtentID'];
      $offset = $lockList[$i]['Offset'];
      $mode = $lockList[$i]['Mode'];
      if ($csID >= 0 and $clID >= 0 and $extentID >= 0 and $offset >= 0) {
            if ($lockType == $mode) {
                $lockCount++;
            }
         }
      }
      return $lockCount;
   }

   public function checkCLLockType($db,$lockMode)
   {
      $cursor = $db -> getSnapshot(SDB_SNAP_TRANSACTIONS_CURRENT,NULL,NULL,NULL);
      $errno = $this -> getErrno();
      if( $errno !== 0 )
      {
         echo "\nFailed to exec snapshot. Errno: ". $errno ."\n";
      }
      $record = $cursor -> next();
      $this->assertEquals(true,!empty($record));
      $lockList = $record['GotLocks'];
      $this->assertNotEmpty($lockList);
      foreach ($lockList as $value ) {
         $this->LockBean($value);
         if ($this->isCLLock()) {
            $this->assertEquals($this-> getMode(),$lockMode,$this-> toString());
         }
      }
   }

   public function insertData($clDB,$recordNum)
   {
      $array = array();
      for ($i=0; $i < $recordNum; $i++) { 
         $arr = array();
         $arr["a"] = $i;
         $arr["b"] = $i;
         array_push($array,$arr);
         $clDB -> insert("{a:".$i.",b:".$i."}");
      }
      return $array;
   }

   public function checkRecords($insertRecods,$cursor)
   {
      $count = 0;
      while ($cursor -> next()) {
         $record = $cursor -> next();
         $arr = $insertRecods[$count++];
         if(!$record["a"] == $arr["a"]){
            return false;
         } 
         if(!$record["b"] == $arr["b"]){
            return false;
         }
      }
      if($count == 20){
         return false;
      } 
   }

   public function LockBean($lockList)
   {
      self::$csID =  $lockList['CSID'];
      self::$clID = $lockList['CLID'];
      self::$extentID = $lockList['ExtentID'];
      self::$offset = $lockList['Offset'];
      self::$mode = $lockList['Mode'];
      self::$count = $lockList['Count'];
   }

   public function isCLLock()
   {
      $result = false;
      if (self::$csID >= 0 and self::$clID >= 0 and self::$clID != 65535 and self::$extentID == -1 and self::$offset == -1) {
         $result = true;
      }
      return $result;
   }

   public function getMode()
   {
      return self::$mode;
   }

   public function toString()
   {
      return "LockBean{"."csID=".self::$csID.",clID=".self::$clID.",extentID=".self::$extentID.",offset=".self::$offset.",mode=".self::$mode."\'".",count=".self::$count."}";
   }

   public static function tearDownAfterClass()
   {
      self::$db -> dropCS(self::$csName);      
      self::$db->close();
   }

}

?>