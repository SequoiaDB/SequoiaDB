/****************************************************
@description:      task operate,warp class
@testlink cases:   
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
class Task
{
   private $db;
   public function __construct( $db )
   {
      $this->db = $db ;
   }
   
   public function listall()
   {
      $num = -1 ;
      $cursor = $this->db->listTask() ;
      $err = $this->db -> getError() ;
      if ( $err['errno'] != 0 )
      {
         echo "Failed to call listTask, error code: ".$err['errno'] ;
         return $num ;
      }
      
      while( $cursor->next() ){
            $num++ ;
      }
      
      $err = $this->db -> getError() ;
      if ( $num == -1 && $err['errno'] == -29 )
      {
         $num = 0;
      } 
      
      return $num ;
   }
   
   public function listbycondition( $condition )
   {
      $exist = false ;
      $cursor = $this->db->listTask( $condition ) ;
      $err = $this->db -> getError() ;
      if ( $err['errno'] != 0 )
      {
         echo "Failed to call listTask, error code: ".$err['errno'] ;
         return $exist ;
      }
      while( $cursor->next() ){
         $exist = true ;
      }
      
      return $exist ;
   }
   
   public function getTaskId( $condition )
   {
      $taskID = -1 ;
      $cursor = $this->db->listTask( $condition ) ;
      $err = $this->db -> getError() ;
      if ( $err['errno'] != 0 )
      {
         echo "Failed to call listTask, error code: ".$err['errno'] ;
         return $taskID ;
      }
      while( $record = $cursor->next() )
      {
         $taskID = $record['TaskID'] ;
      }
      
      return $taskID ;
   }
   
   public function wait( $taskID )
   {
      $err = $this->db->waitTask( $taskID ) ; 
      return $err['errno'] ;     
   }
   
   public function cancle( $taskID, $isAsync )
   {
      $err = $this->db->cancelTask( $taskID, $isAsync ) ; 
      return $err['errno'] ;   
   }
   
}
?>

