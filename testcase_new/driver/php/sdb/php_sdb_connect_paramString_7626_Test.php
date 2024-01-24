/****************************************************
@description:      connect / close / isvalid, base case
@testlink cases:   seqDB-7703 / seqDB-7627 / seqDB-7628
@input:        1 connect, $address: string, $useSSL: true
               2 isvalid for connect
               3 close
               4 isvalid for close
@output:     success
@modify list:
        2016-4-27 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SdbConnect02 extends BaseOperator 
{  
   public function __construct()
   {
      parent::__construct();
   }
   
   function getErrno()
   {
      $this -> err = $this -> db -> getError();
      return $this -> err['errno'];
   }
   
   function connect()
   {  
      $address  = $this -> COORDHOSTNAME .":". $this -> COORDSVCNAME;
		$userName = '';
		$password = '';
		$useSSL   = true;
      $this -> db -> connect( $address, $userName, $password );
   }
   
   function isValid()
   {  
      return $this -> db -> isValid();
   }
   
   function close()
   {
      $this -> db -> close();
   }
   
}

class testSdbConnect02 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SdbConnect02();
   }
   
   function test_closeBegin()
   {
      echo "\n---Begin to close before connect with new user.\n";
      
      self::$dbh -> close(); 
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_validAfterClose()
   {
      echo "\n---Begin to Judge the connection is valid after close.\n";
      
      $status = self::$dbh -> isValid();  
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertFalse( false, $status );
   }
   
   function test_connect()
   {
      echo "\n---Begin to connect with the new user.\n";
      
      self::$dbh -> connect();  
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_validAfterConnect()
   {
      echo "\n---Begin to Judge the connection is valid after connect.\n";
      
      $status = self::$dbh -> isValid();  
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      
      $this -> assertTrue( true, $status );
   }
   
}
?>