/****************************************************
@description:      ReplicaGroupMgr operate, warp class
@testlink cases:   seqDB-7636-7644
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
include 'ReplicaGroup.php';
class ReplicaGroupMgr
{
    private $db ;
    private $groups = array() ;
    private $dataGroups = array() ;
    private $err ;
    
    private $coordGroupName = "SYSCoord";
    private $catalogGroupName = "SYSCatalogGroup";
    private $sysspareGroupName = "SYSSpare";
    
    public function __construct($sdb)
    {
       $this->db = $sdb ;
       $this->groups= array() ;
       $this->getGroups() ;
       $this->getDataGroups() ;
       $this->err = 0 ;
    }
    
    public function __destruct()
    {
    }
    
    public function getError()
    {
       return $this->err;
    }
    
    public function getGroups()
    {
        $cursor = $this->db->listGroup();
        if ( empty( $cursor ) )
        {
            $this->err = $this->db->getError()['errno'];
            return $this->groups;
        }
        
        while( $record = $cursor->next() )
        {
           $group = new ReplicaGroup( $this->db, $record['GroupName'] );
           if ( $group->getError()['errno'] != 0 )
           {
              $this->err = $group->getError()['errno'];
              unset( $this->groups );
              return;
           }
    
           array_push( $this->groups, $group );
        }
        return $this->groups;
    }
    
    public function getDataGroups()
    {   
        for ($i = 0; $i < count($this -> groups); ++$i)
        {
          $groupName = $this->groups[$i]->getName();
          if ( strcmp($groupName, $this -> coordGroupName) != 0 &&
               strcmp($groupName, $this -> catalogGroupName) != 0 &&
               strcmp($groupName, $this -> sysspareGroupName) != 0 )
          {
             array_push( $this -> dataGroups, $this->groups[$i] );
          }
        }
        
        return $this -> dataGroups;
    }
   
   /* ***************************************
   *get dataRG Names
   **************************************** */  
   function getDataGroupNames()
   {     
      $dataGroupNames = array();
      
      for( $i = 0; $i < count($this -> dataGroups); $i++ )
      {
         array_push( $dataGroupNames, $this -> dataGroups[$i] -> getName() );
      }
      
      return $dataGroupNames;
   }
    
    
    public function addDataGroup( $groupName )
    {
        $err = $this->db->createGroup( $groupName );
        if ( $err['errno'] == 0 )
        {
           $group = new ReplicaGroup($this->db, $groupName ) ;
           array_push( $this->groups, $group );
           $this->err = $group->getError();
           return $group ;
        }
        
        $this->err = $err['errno'];
        return ;
    }
    
    public function addCatalogGroup( $hostName, $serviceName, $dbpath, $cfg = NULL )
    {
        if (count($this->groups) > 0) return;
        $err = $this->db->createCataGroup( $hostName, $serviceName, $dbpath, $cfg );
        if ( $err['errno'] == 0 )
        {
           $group = new ReplicaGroup( $this->db, "SYSCatalogGroup" ) ;
           $this->err = $group->getError();
           array_push( $this->groups, $group );
           return $group ;
        }
        $this->err = $err['errno'];
        return ;
    }
    
    public function removeGroup( $groupName )
    {
       if (count($this->groups) > 1 && strcmp($groupName, "SYSCatalogGroup") == 0) return 0;
       for ($i = 0; $i < count($this->groups); ++$i)
       {
          if ( strcmp($this->groups[$i]->getName(), $groupName) == 0 )
          {
             break;
          }
       }
       
       if ( $i == count($this->groups))
       {
           return 0 ;
       }
       
       for (; $i < count($this->groups) - 1; ++$i )
       {
          $this->groups[$i] = $this->groups[$i + 1];
       }
       
       array_pop($this->groups) ;
       $err = $this->db->removeGroup($groupName);
       return $err['errno'];
    }
    
    public function getAllHostNamesOfDeploy()
    {
       $hosts = array();
       for ($i = 0; $i < count($this->groups); ++$i)
       {
          $hostsOfPerGroup = $this->groups[$i]->getHostNameOfDeploy();
          if (count($hosts) == 0) 
          {
             $hosts=$hostsOfPerGroup;
          }
          else
          {
             $result = array_diff_assoc($hosts, $hostsOfPerGroup );
             $hosts = array_merge($hosts, $result);
          }
       }
       
       return $hosts;
    }
    
    public function getGroupNum()
    {
       return count( $this -> groups);
    }
    
    public function getDataGroupNum()
    {
       return count( $this -> dataGroups);
    }
}
?>
