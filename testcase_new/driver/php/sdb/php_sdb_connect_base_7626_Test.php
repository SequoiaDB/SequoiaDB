/****************************************************
@description:      connect, multi address
@testlink cases:   seqDB-7626
@input:        1 connect, multi address
               2 isvalid for connect
               3 close
               4 isvalid for close
@output:     success
@modify list:
        2016-10-18 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SdbConnect03 extends BaseOperator 
{  
   protected $sdb;
   public function __construct()
   {
      parent::__construct();
   }
   
   function getErrno()
   {
      $this -> err = $this -> sdb -> getError();
      return $this -> err['errno'];
   }
   
   function connect( $opt )
   {  
      if( $opt == 'partInvalid' ){
         //"88.88.88.88:11810" is invalid
         $address = array( "88.88.88.88:11810", $this -> COORDHOSTNAME .":". $this -> COORDSVCNAME );
      }else if( $opt == 'allInvalid' )
      {
         //all address are invalid
         $address = array( "88.88.88.88:11810", "88.88.88.89:11810" );
      }
      $this -> sdb = new SequoiaDB();
      $this -> sdb -> connect( $address );
   }
   
   function isValid()
   {  
      return $this -> sdb -> isValid();
   }
   
}

class testSdbConnect03 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SdbConnect03();
   }
   
   function test_connectSuccess()
   {
      echo "\n---Begin to connect, the first address is invalid.\n";
      
      self::$dbh -> connect( 'partInvalid' );  
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_valid()
   {
      echo "\n---Begin to check the valid connect.\n";
      
      $status = self::$dbh -> isValid(  ); 
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
      $this -> assertTrue( true, $status );
   }
   
   function test_connectFailed()
   {
      echo "\n---Begin to connect, the all address is invalid.\n";
      
      self::$dbh -> connect( 'allInvalid' ); 
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -15, $errno );
   }
   
}
?>