/****************************************************
@description:      Procedure test
@testlink cases:   seqDB-7693/7694
@modify list:
        2016-6-13 wenjing wang init
****************************************************/
<?php
   
   include_once dirname(__FILE__).'/lib/Procedure.php';
   include_once dirname(__FILE__).'/lib/comm.php';
   include_once dirname(__FILE__).'/../global.php';
class ProcedureTest extends PHPUnit_Framework_TestCase
{
   private static $db ;
   private static $procedure ;
   private static $skipTestCase ;
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb() ;
      $err = self::$db->connect( globalParameter::getHostName() , 
                                globalParameter::getCoordPort() ) ;
      if ( $err['errno'] != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return ;
      }   
     
      self::$procedure = new Procedure( self::$db ) ;
   }
   
   public function setUp()
   {
      if ( self::$skipTestCase == true )
      {
         $this->markTestSkipped( 'connect failed' );
      }
      
      if ( common::IsStandlone( self::$db ) )
      {
         $this->markTestSkipped( 'database is standlone' ); 
      }
   }
   
   public function testCreate()
   {
      $err = self::$procedure->create( 'function sum7693( a,b ){ return a + b ; }' );
      $this->assertEquals( 0, $err ) ;
      
      //$result = self::$procedure->exec( 'sum7693(1,2)' );
      //$this->assertEquals( 3, $result ) ;
      
      //$ret = self::$procedure->listbyname( 'sum7693' );
      //$this->assertEquals( true, $ret ) ;
   }
   
   /**
    * @depends testCreate
    */
   public function testRemove()
   {
      $err = self::$procedure->remove( 'sum7693' );
      $this->assertEquals( 0, $err ) ;
      
      //$err = self::$procedure->exec( 'sum7693(1,2)' );
      //$this->assertEquals( -152, $err ) ;
   }
   
   /**
    * @depends testRemove
    */
   public function testList()
   {
      //$ret = self::$procedure->listbyname( 'sum7693' );
      //$this->assertEquals( false, $ret ) ;
   }
   
   public static function tearDownAfterClass()
   {
      self::$db->close();
   }
}
?>

