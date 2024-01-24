cription:      ReplicaNode operate, warp class
@testlink cases:   seqDB-7636-7644
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
class ReplicaNode
{
    protected $dbpath ;
    protected $node ;
    
 
    public function __construct($node, $dbpath)
    {
       $this->node = $node ;
       $this->dbpath = $dbpath ;
    }
    
    public function __destruct()
    {
    }
    
    public function getName()
    {
       return $this->node->getName() ;  
    }
    
    public function getHostName()
    {
       $name = $this->node->getHostName();
       return $name ;
    }
    
    public function getServiceName()
    {
       return $this->node->getServiceName();
       
    }
    
    public function getStatus()
    {
       $status = $this->node->getStatus() ;
       return $status;
    }
    
    public function start()
    {
       $err = $this->node->start();
       return $err['errno'];
    }
    
    public function stop()
    {
       $err = $this->node->stop();
       return $err['errno'];
    }
    
    public function connect()
    {
       $nodedb = $this->node->connect();
       return $nodedb;
    }
}
?>

