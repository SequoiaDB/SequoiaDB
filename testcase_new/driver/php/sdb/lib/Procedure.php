/****************************************************
@description:      Procedure warp class
@testlink cases:   
@modify list:
        2016-6-20 wenjing wang init
****************************************************/
<?php
class Procedure
{
   private $db;
   public function __construct($db)
   {
      $this->db = $db;
   }

   public function create($code)
   {
      $err = $this->db->createJsProcedure($code);
      return $err['errno'];
   }

   public function exec($code)
   {
      $result = $this->db->evalJs($code);
      $err = $this->db->getError();
      if ($err['errno'] == 0)
      {
         return $result;
      }
      else
      {
         return $err['errno'];
      }
   }

   public function remove($name)
   {
      $err = $this->db->removeProcedure($name);
      return $err['errno'];
   }
   
   private function buildParameter($name)
   {
      if (mt_rand(0,1) == 1)
      {
         return array('name'=> $name);
      }
      else
      {
         return json_encode(array('name'=> $name));
      }
   }
   public function listbyname($name)
   {
      $exist = false;
      $cursor = $this->db->listProcedure($this->buildParameter($name));
      
      while($record = $cursor->next())
      {
         $exist = true;
      }
      $err = $this->db->getError() ;
      if ( $err['errno'] != -29 ){
         $exist = false ;
         var_dump($err) ;
      }

      return $exist;
   }
}
?>
