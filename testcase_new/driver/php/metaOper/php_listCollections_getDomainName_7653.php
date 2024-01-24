/****************************************************
@description:      SdbCS new interface： listCollections()  getDomainName()
@testlink cases:   seqDB-25367
@modify list:
        2022-02-14 Lantian init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class ListCollections_getDomainName_7653 extends PHPUnit_Framework_TestCase
{
   private static $db;
   private static $cs;
   private static $csName="cs7653";
   
   private static $domain0name="domain0";
   private static $domain1name="domain1";
   
   public function setUp()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'.
                           globalParameter::getCoordPort()) ;
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      // 创建集合
      self::$db -> dropCS(self::$csName);
      self::$db -> createCS(self::$csName);
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      self::$cs = self::$db -> selectCS(self::$csName);
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      
      // 创建domain
      self::$db -> dropDomain(self::$domain0name);
      self::$db -> dropDomain(self::$domain1name);
      
      self::$db -> createDomain(self::$domain0name, array( 'Groups' => array( 'group1', 'group2')));
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      self::$db -> createDomain(self::$domain1name, array( 'Groups' => array( 'group1', 'group2')));
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );

      self::$cs->setDomain(array( 'Domain' =>  self::$domain0name));
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        
   }
   
   function test()
   {
   
        // 测试listCL（）
        
        // 场景1 无CL
        $cls0 = self::$cs -> listCL();
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        
        $count0=0;
        while( $record = $cls0 -> next() ) {
        $count0++;
        }
        $this -> assertEquals( 0,$count0 );
        
        self::$cs->createCL("cl1");
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        
        self::$cs->createCL("cl2");
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        
        // 场景3 多CL        
        $cls1 = self::$cs -> listCL();
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        
        $count1=0;
        while( $record = $cls1 -> next() ) {
        $count1++;
        }
        $this -> assertEquals( 2,$count1 );
        
        // 测试getDomainName（）
        
        // 场景1 正常获取
        $domainName0 = self::$cs -> getDomainName();
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        $this -> assertEquals( self::$domain0name, $domainName0 );
        
        // 场景2，切换domain
        self::$cs -> setDomain( array( 'Domain' => self::$domain1name )) ;
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );

        $domainName1 = self::$cs -> getDomainName();
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        $this -> assertEquals( self::$domain1name, $domainName1 );

        self::$cs -> removeDomain();

        // 场景3，不存在
        $domainName2 = self::$cs -> getDomainName();
        $this -> assertEquals( 0, self::$db -> getError()['errno'] );
        $this -> assertEquals( "", $domainName2 );
   }
   
   public function tearDown()
   {
      //用例执行结束，恢复环境
      
      self::$db -> dropCS(self::$csName);
      
      self::$db -> dropDomain(self::$domain0name);
      self::$db -> dropDomain(self::$domain1name);
            
      self::$db->close();
   }
}
?>