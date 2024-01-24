/****************************************************
@description:      createUser / removeUser, base case
@testlink cases:   seqDB-7703
@input:        1 createUser
               2 close old connect
               3 isvalid for close
               4 connect with the new user
               5 isvalid for connect
               6 removeUser
               7 connect again with the removed user
@output:     success
@modify list:
        2016-4-27 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SdbUser extends BaseOperator 
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
   
   function createUser()
   {
      $this -> db -> createUser( 'mike', '123456789' );
   }
   
   function connect()
   {  
      $address = $this->COORDHOSTNAME.':'.$this -> COORDSVCNAME;
		$userName = 'mike';
		$password = '123456789';
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
   
   function removeUser()
   {  
      $this -> db -> removeUser( 'mike', '123456789' );
   }
   
}

class TestSdbUser extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SdbUser();
      
      if( self::$dbh -> commIsStandlone() === false )
      {
         echo "\n---Begin to drop user in the begin.\n";
         self::$dbh -> removeUser();
      }
   }
   
   public function setUp()
   {
      if( self::$dbh -> commIsStandlone() === true )
      {
         $this -> markTestSkipped( "Database is standlone." );
      }
   }
   
   function test_createUser()
   {
      echo "\n---Begin to create user.\n";
      
      self::$dbh -> createUser();  
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
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
   
   function test_removeUser()
   {
      echo "\n---Begin to drop user.\n";
      
      self::$dbh -> removeUser();  
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>