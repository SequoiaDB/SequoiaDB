/****************************************************
@description:      setSessionAttr
@testlink cases:   seqDB-7703 
@modify list:
        2016-6-13 wenjing wang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';
class flushConfigureTest extends PHPUnit_Framework_TestCase
{
    protected $db;
    protected function setUp()
    {
       $this->db = new Sequoiadb();
       $err = $this->db->connect( globalParameter::getHostName() , 
                                  globalParameter::getCoordPort() ) ;
       $this->assertEquals( 0, $err['errno'] ) ;
    }
    
    public function testFlushConfigure()
    {
        $err = $this->db->flushConfigure( array( 'Global' => true ) );
        $this->assertEquals( 0, $err['errno'] ) ;
    }
    
    protected function tearDown()
    {
        $err = $this->db->close();
        $this->assertEquals( 0, $err['errno'] ) ;
    }
}
?>
