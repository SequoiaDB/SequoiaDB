/****************************************************
@description:      check snapshot options
@testlink cases:   seqDB-16619
@modify list:
        2018-11-13 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class Rename16619 extends PHPUnit_Framework_TestCase
{
   private static $db;
   
   public static function setUpBeforeClass()
   {
      self::$db = new Sequoiadb();
      self::$db -> connect(globalParameter::getHostName().':'. 
                           globalParameter::getCoordPort()) ;
      self::checkErrno( 0, self::$db -> getError()['errno'] );                     
   }
   
   function test()
   {
      echo "\n---Begin to check snapshot.\n";
      //counts the number of nodes in the current cluster
      $csSnapCur = self::$db -> snapshot( SDB_SNAP_HEALTH, 
				array( 'IsPrimary' =>  true, 'Status' => 'Normal' ),
				array( 'ServiceStatus' => 1, 'IsPrimary' => 1),
				array( 'NodeName' => 1),
				null, -1, -1 );
      if( empty( $csSnapCur ) ) 
      {
         throw new Exception('get health snapshot error, is empty');
      }
      $length = 0;
      while( $record = $csSnapCur -> next())
      {
         $length++;
      }
      $returnNum = 1;
      $skipNum = $length - 1;
      
      //check snapshot cs name and returnNum and skipNum
      $csSnapCur = self::$db -> snapshot( SDB_SNAP_HEALTH,           
                                array( 'IsPrimary' =>  true, 'Status' => 'Normal' ),
                                array( 'ServiceStatus' => 1, 'IsPrimary' => 1),
                                array( 'NodeName' => 1),
                                null, $skipNum, $returnNum );
      while( $record = $csSnapCur -> next())
      {
         //check array length
         if( count( $record ) != 2)
         {
            throw new Exception( 'check array length  error, exp: 2  act: ' . count( $record ) );
         }
         //check record contain
         $IsPrimary = $record['IsPrimary'];
         $ServiceStatus = $record['ServiceStatus'];
         $times = 0;
         if( $IsPrimary != true )
         {
            throw new Exception( 'check record error, exp : IsPrimary => true, act IsPrimary => ' . $IsPrimary );
         }
         if( $ServiceStatus != true )
         {
            throw new Exception( 'check record error, exp : ServiceStatus => true, act ServiceStatus => ' . $ServiceStatus );
         }
         //count record num
         $times++;
      }
      if( $times != $returnNum)
      {
         throw new Exception( 'check snapshot record num error, exp: '. $returnNum .', act: ' . $times );
      }
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---check snapshot complete.\n";
      self::$db->close();
   }
   
   private static function checkErrno( $expErrno, $actErrno, $msg = '' )
   {
      if( $expErrno != $actErrno ) 
      {
         throw new Exception( 'expect ['.$expErrno.'] but found ['.$actErrno.']. '.$msg );
      }
   }
   
}
?>
