/****************************************************
@description:      CI input parameters
@modify list:
        2016-11-22 wenjing wang init
****************************************************/
<?php
class globalParameter
{
   static public function getHostName()
   {
      if ( !isset( $_POST['HOSTNAME'] ) && empty( $_POST['HOSTNAME'] ) )
      {
         return 'localhost' ;
      }
      else
      {
         return $_POST['HOSTNAME'] ;
      }
   }

   static public function getCoordPort()
   {
      if ( !isset( $_POST['SVCNAME'] ) && empty( $_POST['SVCNAME'] ) )
      {
         return 50000 ;
      }
      else
      {
         return $_POST['SVCNAME'] ;
      }
   }

   static public function getChangedPrefix()
   {
      if ( !isset( $_POST['CHANGEDPREFIX'] ) && empty( $_POST['CHANGEDPREFIX'] ) )
      {
         return "php_test" ;
      }
      else
      {
         return $_POST['CHANGEDPREFIX'] ;
      }
   }
   static public function getSpareportStart()
   {
      if ( !isset( $_POST['RSRVPORTBEGIN'] ) && empty( $_POST['RSRVPORTBEGIN'] ) )
      {
         return 26000 ;
      }
      else
      {
         return $_POST['RSRVPORTBEGIN'] ;
      }
   }
   
   static public function getSpareportStop()
   {
      if ( !isset( $_POST['RSRVPORTEND'] ) && empty( $_POST['RSRVPORTEND'] ) )
      {
         return 27000 ;
      }
      else
      {
         return $_POST['RSRVPORTEND'] ;
      }
   }
   
   static public function getDbPathPrefix()
   {
      if ( !isset( $_POST['RSRVNODEDIR'] ) && empty( $_POST['RSRVNODEDIR'] ) )
      {
         return "/opt/sequoiadb/database/" ;
      }
      else
      {
         return $_POST['RSRVNODEDIR'] ;
      }
   }

   public static function isStandalone( $db ){
      $db -> listGroup() ;
      $err = $db -> getError();
      if ( $err['errno'] != 0 )
      {
         return true;
      }
      return false;
   }

   public static function checkError( $db, $expErrno, $msg = "" ){
      $actErrno = $db -> getError()['errno'];
      if( $expErrno != $actErrno )
      {
         throw new Exception( 'expect ['.$expErrno.'] but found ['.$actErrno.']. '.$msg );
      }
   }

   // compare two array:if $arr contains all the elements of $targe
   public static function compareArray( $arr, $targe )
   {
      foreach( $targe as $key => $value )
      {
         if( $arr[$key] != $value )
         {
            return false;
         }
      }
      return true;
   }

};
?>
