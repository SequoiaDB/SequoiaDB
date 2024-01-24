/****************************************************
@description:     test resetSnapshot option
@testlink cases:  seqDB-14425
@output:          success
@modify list:
      2018-02-06  Suqiang Ling init
****************************************************/
<?php

include_once dirname(__FILE__).'/../func.php';

class ResetSnapshot14425 extends BaseOperator 
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

   // some place such as setUpBeforeClass and function of BaseOperator, can't use "assert".
   // so I use this function instead of assert.
   function checkErrno( $expErrno, $actErrno, $msg = "" )
   {
      if( $expErrno != $actErrno ) 
      {
         throw new Exception( "expect [".$expErrno."] but found [".$actErrno."]. ".$msg );
      }
   }

   function connectNodeMaster( $rgName )
   {
      $groupObj = $this -> db -> getGroup( $rgName );
      $this -> checkErrno( 0, $this -> getErrno() );
      $nodeObj = $groupObj -> getMaster();
      $this -> checkErrno( 0, $this -> getErrno() );
      $dataDB = $nodeObj -> connect();
      $this -> checkErrno( 0, $this -> getErrno() );
      return $dataDB;
   }

   function createStatisInfo( $dataDB, $csName, $clName )
   {
      $clFullName = $csName.".".$clName;
      $cl = $dataDB -> getCL( $clFullName );
      $this -> checkErrno( 0, $this -> getErrno() );
      $cursor = $cl -> find();
      $this -> checkErrno( 0, $this -> getErrno() );
      while( $record = $cursor -> next() ) {}
   }

   function resetSnapshot( $cond )
   {
      $this -> db -> resetSnapshot( $cond );
      $this -> checkErrno( 0, $this -> getErrno() );
   }

   function isDataBaseSnapClean( $dataDB )
   {
      $cursor = $dataDB -> snapshot( SDB_SNAP_DATABASE );
      $this -> checkErrno( 0, $this -> getErrno() );
      $record = $cursor -> next();
      $this -> checkErrno( 0, $this -> getErrno() );
      $totalRead = $record['TotalRead'];
      return ( $totalRead == "0" );
   }

   function isSessionSnapClean( $dataDB )
   {
      $cursor = $dataDB -> snapshot( SDB_SNAP_SESSIONS, "{Type: 'Agent'}" );
      $this -> checkErrno( 0, $this -> getErrno() );
      $record = $cursor -> next();
      $this -> checkErrno( 0, $this -> getErrno() );
      $totalRead = $record['TotalRead'];
      return ( $totalRead == "0" );
   }
}

class TestResetSnapshot14425 extends PHPUnit_Framework_TestCase
{
   private static $dbh;
   private static $csName;
   private static $clName;
   private static $rgName;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new ResetSnapshot14425();
      if( self::$dbh -> commIsStandlone() )
      {
         echo "\n---Skip standalone mode\n";
         return ;
      }

      // echo "\n---Begin to select rgName.\n";
      // Note: using assert/checkErrno after every operation, is much better than print screen.
      $dataRgNames = self::$dbh -> commGetGroupNames();
      // $this -> assertEquals( 0, self::$dbh -> getErrno() );
      // Note: setUpBeforeClass can't support assert. because static is before $this exists.
      self::$dbh -> checkErrno( -29, self::$dbh -> getErrno() );

      self::$rgName = $dataRgNames[0];
      $options = array( 'Group' => self::$rgName );
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME.'14425';

      $cl = self::$dbh -> commCreateCL( self::$csName, self::$clName, $options, true );
      self::$dbh -> checkErrno( 0, self::$dbh -> getErrno() );

      for( $i = 0; $i < 100; $i++ )
      {
         $cl -> insert( "{ a: 1 }" );
         self::$dbh -> checkErrno( 0, self::$dbh -> getErrno() );
      }
   }
   
   function test()
   {
      if( self::$dbh -> commIsStandlone() )
         return ;
      $dataDB = self::$dbh -> connectNodeMaster( self::$rgName );

      self::$dbh -> createStatisInfo( $dataDB, self::$csName, self::$clName);
      self::$dbh -> resetSnapshot( "{ Type: 'sessions' }" );

      $this -> assertFalse( self::$dbh -> isDataBaseSnapClean( $dataDB ) );
      $this -> assertTrue( self::$dbh -> isSessionSnapClean( $dataDB ) );
      $dataDB -> close();
   }
   
   public static function tearDownAfterClass()
   {
      if( self::$dbh -> commIsStandlone() )
         return ;
      $err = self::$dbh -> commDropCL( self::$csName, self::$clName, false );
      self::$dbh -> checkErrno( 0, $err );
   }
   
}
?>
