/****************************************************
@description:      install, base case
@testlink cases:   seqDB-7722
@input:        1 install, $options: null/array/string
@output:     success
@modify list:
        2016-4-27 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SdbInstall extends BaseOperator 
{  
   public function __construct()
   {
      parent::__construct();
   }
   
   function install( $returnType )
   {  
      if( $returnType === null )
      {
         $options = null;
      }
      else if( $returnType === 'array' )
      {
         $options = array( 'install' => true );
      }
      else if( $returnType === 'string' )
      {
         $options = '{ "install": false }';
      }
      
      $this -> db -> install( $options );
      $errInfo = $this -> db -> getError();
      return $errInfo;
   }
   
}

class testSdbInstall extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SdbInstall();
   }
   
   function test_installForNull()
   {
      echo "\n---Begin to exec install[null].\n";
      
      $errInfo = self::$dbh -> install( null ); 
      
      $this -> assertEquals( 0, $errInfo['errno'] );
      $this -> assertTrue( is_array( $errInfo )  ) ;
   }
   
   function test_installForString()
   {
      echo "\n---Begin to exec install[string].\n";
      
      $errInfo = self::$dbh -> install( 'string' ); 
      
      $this -> assertEquals( '{"errno":0}', $errInfo );
      $this -> assertTrue( is_string( $errInfo )  ) ;
   }
   
   function test_installForArray()
   {
      echo "\n---Begin to exec install[array].\n";
      
      $errInfo = self::$dbh -> install( 'array' ); 
      
      $this -> assertEquals( 0, $errInfo['errno'] );
      $this -> assertTrue( is_array( $errInfo )  ) ;
   }
   
}
?>