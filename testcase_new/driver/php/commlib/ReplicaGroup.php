/****************************************************
@description:      ReplicaGroup operate, warp class
@testlink cases:   seqDB-7636-7644
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
include 'ReplicaNode.php';
class ReplicaGroup
{
    protected $db ;
    protected $name ;
    protected $group ;
    protected $nodes ;
    protected $PrimaryNode ;
    protected $err ;
   
    public function getError()
    {
       return $this->err;
    }
    
    public function __construct( $sdb, $groupName = "" )
    {
       $this->db = $sdb ;
       $this->name = $groupName ;
       $this->err = 0 ;
       $this->nodes= array() ;
       $this->init() ;
    }
    
    public function __destruct()
    {
    }
    
    private function init()
    {
       if ( strlen( $this->name ) )
       {
          $this->group = $this->db->getGroup( $this->name ) ;
          if( empty( $this->group ) ) 
          {
             $error = $this->db -> getError() ;
             
             $this->err = $error['errno'];
          }
          else
          {
             $this->getNodes() ;
          }
       }
       else
       {
          $this->err = -154 ;
       }
       
       return $this->err ;
    }
    
    public function isCataLog()
    {
       if ( isset($this->group) )
       {
          $this->init();
       }
       
       if ($this->err != 0)
       {
          return ;
       }
       
       $ret = $this->group->isCatalog();
       return $ret ;
    }
    
    public function getName()
    {
        return $this->name;
    }
    
    public function reelect( $options = NULL )
    {
        $err = $this->group->reelect( $options );
        $this->err = $err['errno'] ;
        return $err['errno'];
    }
    
    public function getNodeNum()
    {
       if (isset($this->nodes) == false) 
       {
          return 0 ;
       }
       return count( $this->nodes );
    }
    
    public function getNodes()
    {
        $detail = $this->group->getDetail();
        $this->err = $this->db -> getError()['errno'];
        if ($this->err != 0 ) return;
        $nodesOfGroup = $detail['Group'];
        if ( array_key_exists('PrimaryNode', $detail) )
        {
           $this->PrimaryNode = $detail['PrimaryNode'];
        }
        
        for ( $i=0; $i<count($nodesOfGroup); ++$i )
        {
           $tmp = $nodesOfGroup[$i];
           $node = $this->group->getNode($tmp['HostName'].":".$tmp['Service'][0]['Name']);
           $this->err = $this->db->getError()['errno']; 
           if ( $this->err != 0 )
           {
              unset( $this->nodes ) ;
              return ;
           }
           $repliNode = new ReplicaNode($node, $tmp['dbpath'] );
           
           array_push($this->nodes, $repliNode) ;
        }
        
        return $this->nodes;
    }
    
    public function getMaster()
    {
        $node = $this->group->getMaster();
        for ($i=0; $i<count($this->nodes); ++$i)
        {
            $tmpnode = $this->nodes[$i];
            if ($node->getHostName() == $tmpnode->getHostName() && 
                $node->getServiceName() == $tmpnode->getServiceName())
            {
               break;
            }
        }
        
        if ($i != count($this->nodes))
        {
           return $node;
        }
        else 
        {
           return NULL;
        }
    }
    
    public function getSlave()
    {
        $node = $this->group->getSlave();
        for ($i=0; $i<count($this->nodes); ++$i)
        {
            $tmpnode = $this->nodes[$i];
            if ($node->getHostName() == $tmpnode->getHostName() && 
                $node->getServiceName() == $tmpnode->getServiceName())
            {
               break;
            }
        }
        
        if ($i != count($this->nodes))
        {
           return $node;
        }
        else 
        {
           return NULL;
        }
    }
   
    public function start()
    {
       $err = $this->group->start();
       return $err['errno'];
    }
    
    public function stop()
    {
       $err = $this->group->stop();
       return $err['errno'];
    }
    
    public function getNode($name)
    {
        for ($i=0; $i<count($this->nodes); $i++)
        {
           $repliNode = $this->nodes[$i];
           if ($repliNode->getName() == $name){
              return $repliNode;
           }
        }
        
        return;
    }
    
    public function addNode( $hostName, $serviceName, $dbpath, $cfg = NULL )
    {
        $err = $this->group->createNode( $hostName, $serviceName,
                                        $dbpath, $cfg );
                                        
        if ($err['errno'] == 0)
        {
           $node = $this->group->getNode( $hostName.":".$serviceName );
           if ( empty( $node ) )
           {
              //$err = $this->
              echo "empty node\n";
           }
           else
           {    
              $repliNode = new ReplicaNode($node, $dbpath);
              array_push( $this->nodes, $repliNode );
           }
        }
        
        return $err['errno'];
    }
    
    private function removeWrapNode( $hostName, $serviceName )
    {
        $exist = false ;
        for ( $i = 0; $i < count( $this->nodes ); ++$i )
        {
           $node = $this->nodes[$i];
           if ( strcmp($node->getHostName(), $hostName ) == 0 &&
               strcmp($node->getServiceName(),$serviceName) == 0 )
           {
              $exist = true;
              break;
           }
        }
       
        for(; $i<count($this->nodes) - 1;++$i)
        {
            $this->nodes[$i] = $this->nodes[$i+1];
            
        }

        if ($exist)
        {
           array_pop($this->nodes) ;   
        }
    }

    public function removeNode( $hostName, $serviceName )
    {
        if (isset($this->nodes) == false) return 0;
        $exist = false;
        for ( $i = 0; $i < count( $this->nodes ); ++$i )
        {
           $node = $this->nodes[$i];
           if ( strcmp($node->getHostName(), $hostName ) == 0 &&
               strcmp($node->getServiceName(),$serviceName) == 0 )
           {
              $exist = true;
              break;
           }
        }
        
        if ($exist == false) return 0;
        if ( empty( $this->cfg ) )
        {
           $err = $this->group->removeNode( $node->getHostName(), $node->getServiceName() );
        }
        else
        {
           $err = $this->group->removeNode( $node->getHostName(), $node->getServiceName(), $this->cfg );
        }
        
        if ($err['errno'] == 0)
        {
           
           for(; $i<count($this->nodes) - 1;++$i)
           {
              $this->nodes[$i] = $this->nodes[$i+1];
           }
           
           array_pop($this->nodes) ;
        }
        return $err['errno'];
    }
    
    public function attachNode( $node, $options=NULL )
    {
        $err = $this->group->attachNode( $node->getHostName(), $node->getServiceName(), $options );
        
        if (0 == $err['errno'])
        {
           array_push($this->nodes, $node) ;
        }
        return $err['errno'];
    }
    
    public function detachNode( $node, $options=NULL )
    {
        $err = $this->group->detachNode( $node->getHostName(), $node->getServiceName(), $options );
        if (0 == $err['errno'])
        {
           $this->removeWrapNode( $node->getHostName(), $node->getServiceName()) ;
        }
        return $err['errno'];
    }
    
    public function getHostNameOfDeploy()
    {
       $hosts = array() ;
       for ($i = 0; $i < count($this->nodes); ++$i)
       {
          for ($k = 0; $k < count($hosts); ++$k)
          {
             if (strcmp($hosts[$k], $this->nodes[$i]->getHostName()) == 0)
             {
                break;
             }
          }
          if ($k == count($hosts))
          {
             array_push($hosts, $this->nodes[$i]->getHostName()) ;
          }
       }
       
       return $hosts;
    }
}
?>

