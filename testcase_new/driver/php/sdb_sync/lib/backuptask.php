/****************************************************
@description:      backup operate,warp class
@testlink cases:   
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
class BackupTask
{
   private $db;
   public function __construct( $db )
   {
      $this->db = $db ;
   }
   
   public function backupOffline( $options )
   {
      $err = $this->db->backupOffline( $options ) ;
      return $err['errno'] ;
   }
   
   public function backup( $options )
   {
      $err = $this->db->backup( $options ) ;
      return $err['errno'] ;
   }
   
   public function removeBackup( $options )
   {
      $err = $this->db->removeBackup( $options ) ;
      return $err['errno'] ;
   }
   
   public function listBackup( $options )
   {
      $exist = false ;
      $cursor = $this->db->listBackup( $options ) ;
      $err = $this->db->getError() ;
      if ( $err['errno'] != 0 ) 
      {
         echo "Failed to call listBackup, error code: ".$err['errno'] ;
         return $exist ;
      }
      
      while( $record = $cursor->next() )
      {
         $exist = true ;
      }
      
      return $exist ;
   }
    
}
?>
