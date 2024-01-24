/****************************************************
@description:      lob operate, warp class
@testlink cases:   seqDB-7681-7689
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
class Lob
{
   private $db ;
   private $cl ;
   private $lob ;
   private $wbuf ;
   private $rbuf ;
      
   public function __construct( $db, $cl )
   {
      $this->db = $db ;
      $this->cl = $cl ;
   }
   
   public function getWContent()
   {
      return $this->wbuf;
   }
   
   public function getRContent()
   {
      return $this->rbuf ;
   }

   public function open( $oid, $mode )
   {
      $this->lob = $this->cl->openLob( $oid, $mode ) ;
      $err = $this->db->getError() ;
      return $err['errno'] ;
   }
   
   public function remove($oid)
   {
      $err = $this->cl->removeLob( $oid ) ;
      return $err['errno'] ;
   }
   
   private function getRandomStr( $len )
   {
      $str = "" ;
      for ( $i=0 ; $i<$len ; $i++ )
      {
         $val = mt_rand( 33, 126 ) ;
         $ch = chr( $val ) ;
         $str=$str.$ch ;
      }
      return $str ;
   }

   public function write( $len )
   {    
     /* if (empty($this->lob))
      {
         var_dump($this->lob);
         $this->lob = new SequoiaLob();
      }*/
      
      $str = $this->getRandomStr( $len ) ;
      //var_dump($str);
      $err = $this->lob->write( $str ) ;
      if ($err['errno'] == 0)
      {
         $this->wbuf = $this->wbuf.$str ;
      }
       
      return $err['errno'] ;
   }
   
   public function read( $len = 0 )
   {  
      if ( empty( $this->lob ) )
      {
         $this->lob = new SequoiaLob() ;
      }
      
      if ( $len == 0 )
      {
         $len = $this->lob->getSize() ;
      }
      $buf = $this->lob->read( $len ) ;
      //var_dump($buf);
   
      $err = $this->db->getError() ;
      if ( $err['errno'] == 0 )
      {
         $this->rbuf = $this->rbuf.$buf ;
      }
      return $err['errno'] ;
   }
  
   public function seek( $offset, $pos )
   {
      $err = $this->lob->seek( $offset, $pos ) ;
      return $err['errno'] ;
   }
   
   public function closeLob()
   {
      $err = $this->lob->close() ;
      return $err['errno'] ;
   }
   
   public function getCreateTime()
   {
      $time = $this->lob->getCreateTime() ;
      return $time ;
   }
}
?>

