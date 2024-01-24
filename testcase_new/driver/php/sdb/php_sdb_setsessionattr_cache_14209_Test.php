/****************************************************
@description:      setSessionAttr
@testlink cases:   seqDB-14209
@modify list:
        2018-1-23 xiaoni huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../commlib/ReplicaGroupMgr.php';
include_once dirname(__FILE__).'/../global.php';
class setSessionAttr14209 extends PHPUnit_Framework_TestCase
{
   protected static $db ;
   private static $groupMgr; 
   protected static $clDB ;
   protected static $csName  = 'cs14209';
   protected static $clName  = 'cl';
   private static $skipTestCase = false;  

   public static function setUpBeforeClass()
   {
      // connect
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '');
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
      
      // judge groupNum
      self::$groupMgr = new ReplicaGroupMgr(self::$db);
      if ( self::$groupMgr -> getError() != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return;
      } 
      
      if (self::$groupMgr -> getDataGroupNum() < 1)
      {
         self::$skipTestCase = true ;
         return;
      } 
      
      // create cs
      $csDB = self::$db -> selectCS( self::$csName, null );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".self::$db -> getError()['errno']);
      }
      
      // create cl
      $option = array( 'ReplSize' => 0 );
      self::$clDB = $csDB -> selectCL( self::$clName, $option );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // insert
      self::$clDB -> insert('{a:1}');
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to in, errno=".self::$db -> getError()['errno']);
      }
   }
   
   public function setUp()
   {
      if( self::$skipTestCase === true )
      {
         $this -> markTestSkipped( "init failed" );
      }
   }

   public function test_setSessionAttrNoCache()
   {  
      echo "\n---Begin to setSessionAttr[noCache, first].\n"; 
      $instanceid = 'M';
      $instanceMode = 'random';
      $instanceTimeout = 200000;          
      // setSessionAttr
      $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'PreferedInstanceMode' => $instanceMode
            , 'Timeout' => $instanceTimeout ) );
      $this -> assertEquals( 0, $err['errno'] );      
      // getSessionAttr
      echo "   Begin to getSessionAttr.\n"; 
      $results = self::$db -> getSessionAttr();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
      $this -> assertEquals( (string)$instanceTimeout, $results['Timeout'] );
      
      
      echo "\n---Begin to setSessionAttr[noCache, second].\n"; 
      $instanceid = 'S';
      $instanceMode = 'ordered';
      $instanceTimeout = 400000;            
      // setSessionAttr
      $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'PreferedInstanceMode' => $instanceMode
            , 'Timeout' => $instanceTimeout ) );
      $this -> assertEquals( 0, $err['errno'] );            
      // getSessionAttr
      echo "   Begin to getSessionAttr.\n"; 
      $results = self::$db -> getSessionAttr();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
      $this -> assertEquals( (string)$instanceTimeout, $results['Timeout'] );
   }

   public function test_setSessionAttrCache()
   {  
      echo "\n---Begin to setSessionAttr[cache].\n"; 
      $instanceid = 1;
      $instanceMode = 'random';
      $instanceTimeout = 600000; 
      
      // setSessionAttr
      $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'PreferedInstanceMode' => $instanceMode
            , 'Timeout' => $instanceTimeout ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      // getSessionAttr
      echo "   Begin to getSessionAttr.\n"; 
      for ($i = 0; $i < 10; $i++) {
         $results = self::$db -> getSessionAttr();
         $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
         $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
         $this -> assertEquals( (string)$instanceTimeout, $results['Timeout'] );
      }
   }
   
   public static function tearDownAfterClass()
   {
      if ( self::$skipTestCase == false )
      {
         self::$db -> setSessionAttr( array( 'Timeout' => -1 ) );
         $err = self::$db -> dropCS( self::$csName ); 
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
      }
      
      $err = self::$db -> close();
   }
};
?>