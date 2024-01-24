/****************************************************
@description:      execSQL / execUpdateSQL, base case
@testlink cases:   seqDB-7669
@input:        1 create cs by sql
               2 create cl by sql
               3 insert by sql
               4 select by sql
               5 drop cl by sql
               6 drop cs by sql
@output:     success
@modify list:
        2016-4-27 XiaoNi Huang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class SqlOperator extends BaseOperator 
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
   
   function sqlCreateCS( $csName )
   {
      $sql = 'create collectionspace '. $csName;
      $this -> db -> execUpdateSQL( $sql );
   }
   
   function sqlCreateCL( $csName, $clName )
   {
      $sql = 'create collection '. $csName .".". $clName;
      $this -> db -> execUpdateSQL( $sql );
   }
   
   function sqlInsert( $csName, $clName )
   {
      $this -> db -> execUpdateSQL( 'insert into '. $csName .'.'. $clName .'(a, b) values(1,"test")' );
      $this -> db -> execUpdateSQL( 'insert into '. $csName .'.'. $clName .'(a, b) values(3,"test")' );
      $this -> db -> execUpdateSQL( 'insert into '. $csName .'.'. $clName .'(a, b) values(6,"hello")' );
      $this -> db -> execUpdateSQL( 'insert into '. $csName .'.'. $clName .'(a, b) values(10,"hello")' );
   }
   
   function sqlSelect( $csName, $clName )
   {
      $sql = ' select avg(a) as avg_a, b from '. $csName .".". $clName .' group by b order by b desc';
      $cursor = $this -> db -> execSQL( $sql );
      
      $recsArray = array();
      while( $records = $cursor -> next() )
      {
         array_push( $recsArray, $records );
      }
      
      return $recsArray;
   }
   
   function sqlDropCL( $csName, $clName )
   {
      $sql = 'drop collection '. $csName .".". $clName;
      $this -> db -> execUpdateSQL( $sql );
   }
   
   function sqlDropCS( $csName )
   {
      $sql = 'drop collectionspace '. $csName;
      $this -> db -> execUpdateSQL( $sql );
   }
   
}

class TestSql extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new SqlOperator();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName  = self::$dbh -> COMMCSNAME;
      self::$clName  = self::$dbh -> COMMCLNAME;
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> sqlDropCS( self::$csName, self::$clName );
   }
   
   function test_sqlCreateCS()
   {
      echo "\n---Begin to create cs by sql.\n";
      
      self::$dbh -> sqlCreateCS( self::$csName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_sqlCreateCL()
   {
      echo "\n---Begin to create cl by sql.\n";
      
      self::$dbh -> sqlCreateCL( self::$csName, self::$clName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_sqlInsert()
   {
      echo "\n---Begin to exec insert by sql.\n";
      
      self::$dbh -> sqlInsert( self::$csName, self::$clName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_sqlSelect()
   {
      echo "\n---Begin to exec select by sql.\n";
      
      $recsArray = self::$dbh -> sqlSelect( self::$csName, self::$clName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertEquals( 2, $recsArray[0]['avg_a'] );
      $this -> assertEquals( 'test', $recsArray[0]['b'] );
      $this -> assertEquals( 8, $recsArray[1]['avg_a'] );
      $this -> assertEquals( 'hello', $recsArray[1]['b'] );
   }
   
   function test_sqlDropCL()
   {
      echo "\n---Begin to drop cl by sql.\n";
      
      self::$dbh -> sqlDropCL( self::$csName, self::$clName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_sqlDropCS()
   {
      echo "\n---Begin to drop cs by sql.\n";
      
      self::$dbh -> sqlDropCS( self::$csName ); 
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
}
?>