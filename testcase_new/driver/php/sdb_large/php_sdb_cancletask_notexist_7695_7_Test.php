/****************************************************
@description:      test task
@testlink cases:   seqDB-7695-7697
@modify list:
        2016-6-21 wenjing wang init
****************************************************/

<?php
   
   include_once dirname(__FILE__).'/lib/task.php';
   include_once dirname(__FILE__).'/lib/comm.php';
   include_once dirname(__FILE__).'/../global.php';
class taskTest extends PHPUnit_Framework_TestCase
{
    private static $db;
    private static $task;
    private static $cs;
    private static $cl;
    private static $needtest;
    private static $fullname;
    private static $skipTestCase = false ;
    
    private static function loadData()
    {
       $number = 10000;
       $batchSize = 1000;
       $id = 0;
       for ($i = 0; $i < $number; $i += $batchSize)
       {
          $docs = array();
          for ($j = 0; $j < $batchSize ; $j++)
          {
             $doc = array();
             $doc['_id'] = $id;
             $doc['a'] = $id;
             $doc['b'] = 1;
             $doc['d'] = 'zzz';
             $id = $id + 1;
             array_push($docs, $doc) ;
             //self::$cl->insert(doc);
          } 
          
          $err = self::$cl->bulkInsert( $docs ) ;
          if( $err['errno'] != 0 ) {
            self::$skipTestCase === true ;
            echo "Failed to insert records, error code: ".$err['errno'] ;
            return ;
          }
       }
    }
    
    private static function getSrcGroup($fullname)
    {
       $cursor = self::$db -> snapshot(SDB_SNAP_CATALOG, array('Name' => $fullname));
       $err = self::$db -> getError() ;
       if ($err['errno'] != 0)
       {
          self::$skipTestCase === true ;
          echo "Failed to call snapshot, error code: ".$err['errno'] ;
          return "" ;
       }

       while ($record = $cursor->next())
       {
          var_dump( $record ) ;
          return $record['CataInfo'][0]['GroupName'];
       }
       
       return "" ;
    }
    
    private static function getDestGroup($groupname)
    {
        $cursor = self::$db->list(SDB_LIST_GROUPS);
        $err = self::$db -> getError() ;
        if ($err['errno'] != 0)
        {
          self::$skipTestCase === true ;
          echo "Failed to call snapshot, error code: ".$err['errno'] ;
          return "" ;
        }
        while ($record = $cursor->next())
        {
           $name = $record['GroupName'];
           if ($name != "SYSSpare" && $name != $groupname &&
               $name != "SYSCatalogGroup" && $name != "SYSCoord")
           {
              break;
           }
        }
        
        return $name;
    }
    
    private static function asyncSplit($srcname, $destname)
    {
        if (empty($srcname) ||
            empty($destname))
        {
           self::$skipTestCase = true ;
           return;
        }
        else
        {
           self::$cl -> splitAsync($srcname, $destname, 50);
           self::$needtest = true;
        }
    }
    
    protected function setUp()
    {
       if( self::$skipTestCase === true )
       {
          $this -> markTestSkipped( "connect failed" );
       }
       if (common::IsStandlone(self::$db))
       {
          $this->markTestSkipped('database is standlone'); 
       }
    }
    
    public static function setUpBeforeClass()
    {
       self::$db = new SequoiaDB();
       
       $err = self::$db -> connect(globalParameter::getHostName(), 
                                   globalParameter::getCoordPort());
       if ( $err['errno'] != 0 )
       {
          echo "Failed to connect database, error code: ".$err['errno'] ;
          self::$skipTestCase = true ;
          return;
       }                                   
       if (common::IsStandlone(self::$db)) return;
       
       self::$task = new Task(self::$db);
       $rnd = mt_rand(0,1000); 
       $name = globalParameter::getChangedPrefix().$rnd;
       $err = self::$db ->createCS($name);
       if( $err['errno'] != 0 ) 
       {
           echo "Failed to call createCS, error code: ".$err['errno'] ;
           self::$skipTestCase = true ;
           return ;
       }
       
       self::$cs = self::$db -> selectCS($name);
       $err = self::$db -> getError() ;
       if( $err['errno'] != 0 ) 
       {
          echo "Failed to call selectCS, error code: ".$err['errno'] ;
          self::$skipTestCase = true ;
          return;
       }
       
       $err = self::$cs->createCL($name, json_encode(array('ShardingType' => 'hash', 'ShardingKey' =>array('_id' => 1))));
       if( $err['errno'] != 0 ) {
          echo "Failed to create collection, error code: ".$err['errno'] ;
          self::$skipTestCase = true ;
          return ;
       }
       
       self::$cl = self::$cs -> getCL( $name ) ;
       
       self::$fullname = $name.'.'.$name;
       self::loadData();
       $srcgroupname = self::getSrcGroup(self::$fullname);
       $destgroupname = self::getDestGroup($srcgroupname);
       
       self::asyncSplit($srcgroupname, $destgroupname);
    }

    public function testSelectParameter()
    {
       $selector = mt_rand(0,2);
       return $selector;
    }
    
    public function testlist()
    {
       $ret = self::$task->listbycondition(json_encode(array('Name' => self::$fullname)));
       $this->assertEquals(true, $ret);
       
       $ret = self::$task->listbycondition(array('Name' => self::$fullname));
       $this->assertEquals(true, $ret);
       
    }
    /**
     * @depends testSelectParameter
     *
     */
    public function testWait($selector)
    {
       $taskID = self::$task->getTaskId(array('Name' => self::$fullname));
       if ($selector == 0)
       {
          $ret = self::$task->wait(array($taskID));
       }
       else if ($selector == 1)
       {
          $ret = self::$task->wait(new SequoiaInt64($taskID));
       }
       else
       {
          $ret = self::$task->wait($taskID);
       }
       
       $this->assertEquals(0, $ret);
    }
    
    /**
     * @depends testSelectParameter
     *
     */
    public function ntestcancle($selector)
    {
        $taskID = self::$task->getTaskId(array('Name' => $fullname));
        if ($selector == 0)
        {
          $ret = self::$task->cancle($taskID,false);
        }
        else
        {
          $ret = self::$task->cancle(new SequoiaInt64($taskID),true);
        }
        
        $this->assertEquals(0, $err);
    }

    public static function tearDownAfterClass()
    {
       if (common::IsStandlone(self::$db))
       {
          self::$db->close();
          return ;
       }
       else
       {
          $name = strtok(self::$fullname, '.'); 
          self::$db->dropCS($name);
          self::$db->close();
       }
    }
}
?>


