/****************************************************
@description:      cancel not exist task
@testlink cases:   seqDB-7698
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
    private static $skipTestCase = false;
    public static function setUpBeforeClass()
    {
       self::$db = new SequoiaDB();
       $err = self::$db->connect( globalParameter::getHostName() , 
                                  globalParameter::getCoordPort() ) ;
       if ( $err['errno'] != 0 )
       {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return ;
       }                                    
       self::$task = new Task(self::$db);
    }
    
    public function setUp()
    {
       if ( self::$skipTestCase == true )
       {
         $this->markTestSkipped( 'connect failed' );
       }
       
       if ( common::IsStandlone(self::$db) )
       {
          $this->markTestSkipped('database is standlone'); 
       }
    }

    public function testListTaskOfNoTask()
    {
       $ret = self::$task->listAll();
       $this->assertTrue($ret != -1);
    }
    
    /**
     * @depends testListTaskOfNoTask
     *
     */
    public function testCancleOfNotExist()
    {
       $taskID = 10000000;
       $err = self::$task->cancle($taskID, true);
       $this->assertEquals(-173, $err);

       $err = self::$task->cancle(new SequoiaInt64( '10000000' ), true);
       $this->assertEquals(-173, $err);
    }

    public static function tearDownAfterClass()
    {
       self::$db->close();
    }
}
?>
