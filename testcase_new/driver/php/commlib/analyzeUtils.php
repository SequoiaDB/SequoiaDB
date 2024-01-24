/****************************************************
@description:     common function for analyze
@modify list:
      2018-03-21  Suqiang Ling init
****************************************************/
<?php
class analyzeUtils
{
   // assert message has no line number. that's bad. so I use Exception instead.
   public static function checkErrno( $expErrno, $actErrno, $msg = "" )
   {
      if( $expErrno != $actErrno ) 
      {
         throw new Exception( "expect [".$expErrno."] but found [".$actErrno."]. ".$msg );
      }
   }

   public static function isStandAlone( $db )
   {
      $db -> list( SDB_LIST_GROUPS );
      if( $db -> getError() ['errno'] == -159 )
         return true;
      else if ( $db -> getError() ['errno'] == 0 )
         return false;
      else
         throw new Exception( "unexpected error ".$err );
   }

   public static function getDataGroups( $db )
   {
      $cursor = $db -> list( SDB_LIST_GROUPS );
      self::checkErrno( 0, $db -> getError()['errno'] );
      
      $groupNames = array();
      while( $groupInfo = $cursor -> next() )
      {
         $groupName = $groupInfo['GroupName'];
         if( $groupName !== "SYSCatalogGroup" && 
             $groupName !== "SYSCoord"   && 
             $groupName !== "SYSSpare" )
         {
            array_push( $groupNames, $groupName );
         }
      }
      
      return $groupNames;
   }

   public static function createCSCL( $db, $csName, $clNumPerCs )
   {
      $db -> createCS( $csName );
      self::checkErrno( 0, $db -> getError()['errno'] );
      $cs = $db -> getCS( $csName );
      self::checkErrno( 0, $db -> getError()['errno'] );
      
      $clArray = array();
      for( $i = 0; $i < $clNumPerCs; $i++ )
      {
         $clName = $csName ."_". $i;
         $cs -> createCL( $clName );
         self::checkErrno( 0, $db -> getError()['errno'] );
         $cl = $cs -> getCL( $clName );
         self::checkErrno( 0, $db -> getError()['errno'] );
         array_push( $clArray, $cl );
      }
      return $clArray;
   }

   public static function insertDataWithIndex( $cl )
   {
      $recs = array();
      $recNum = 20000;
      for( $i = 0; $i < $recNum; $i++ )
         array_push( $recs, array( 'a' => 0 ) );
      for( $i = 0; $i < $recNum; $i++ )
         array_push( $recs, array( 'a' => rand( 0, 10000 ) ) );
      $err = $cl -> bulkInsert( $recs, 0 );
      self::checkErrno( 0, $err ['errno'] );

      $err = $cl -> createIndex( array( 'a' => 1 ), 'aIndex' );
      self::checkErrno( 0, $err ['errno'] );
   }

   public static function checkScanTypeByExplain( $db, $cl, $expScanType )
   {
      $cond = '{ "a": 0 }';
      $opt = '{ "Run": true }';
      $cursor = $cl -> explain( $cond, /*selector*/null, /*orderBy*/null, /*hint*/null,
                              /*numToSkip*/0, /*numToReturn*/-1, /*flag*/0, $opt );
      self::checkErrno( 0, $db -> getError() ['errno'] );
      $rec = $cursor -> next();
      $actScanType = $rec['ScanType'];
      if( $actScanType != $expScanType )
      {
         $clName = $cl -> getName();
         self::checkErrno( 0, $db -> getError() ['errno'] );
         throw new Exception( "wrong scanType. cl: ". $clName
               ." expect: ". $expScanType ." actual: ". $actScanType );
      }
   }
}
?>
