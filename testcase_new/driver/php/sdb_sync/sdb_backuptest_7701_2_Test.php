/****************************************************
@description:      backup operate,warp class
@testlink cases:   
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
   
   include_once dirname(__FILE__).'/lib/backuptask.php';
   include_once dirname(__FILE__).'/../global.php';
class backuptest extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $backupTask;
   private static $options;
   private static $skipTestCase = false;
   public static function setUpBeforeClass()
   {
      self::$db = new SequoiaDB();
      $coordHostName = globalParameter::getHostName();
      $coordPort = globalParameter::getCoordPort();
      $err = self::$db->connect($coordHostName, $coordPort);
      if( $err['errno'] != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true;
      }
      self::$backupTask = new BackupTask(self::$db);
   }
   
   public function setUp()
   {
      if( self::$skipTestCase === true )
      {
         $this -> markTestSkipped( "connect failed" );
      }
   }

   public function testbackup()
   {
      $options = array('Name' => 'backupName7701');
      $ret = self::$backupTask->backupOffline($options);
      $this->assertEquals(0, $ret);
      $ret = self::$backupTask->listBackup($options);
      $this->assertEquals(true, $ret);

      self::$backupTask->removeBackup($options);
      self::$options = json_encode($options);
      $ret = self::$backupTask->backupOffline(self::$options);
      $this->assertEquals(0, $ret);

      $ret=self::$backupTask->listBackup(self::$options);
      $this->assertEquals(true, $ret);
      
   }

   /*
    * @depends testbackup 
    *
    */
   public function testremove()
   {
      $ret = self::$backupTask->removeBackup(self::$options);
      $this->assertEquals(0, $ret);
     
      $ret = self::$backupTask->listBackup(self::$options);
      $this->assertEquals(false, $ret);
      
      $ret = self::$backupTask->listBackup(null);
      $this->assertEquals(false, $ret);
   }
   
   public static function tearDownAfterClass()
   {
      self::$db->close();
   }
}
?>
