<?php 

include_once dirname(__FILE__).'/global.php';

class BaseOperator
{  
   protected $db;
   protected $err;
   protected $ignoreNotExist;
   protected $groupNames = array();
   
   //coord param
   protected $COORDHOSTNAME;  //random value from CI.
   protected $COORDSVCNAME;  //random value from CI.
   //group param
   protected $CATALOG_GROUPNAME;
   protected $COORD_GROUPNAME;
   protected $SPARE_GROUPNAME;
   //metadata param
   public $CHANGEDPREFIX;   //random value from CI.
   public $COMMDOMAINNAME;  
   public $COMMCSNAME;
   public $COMMCLNAME;
   
   protected function init()
   {  
      //hostname
      $this -> COORDHOSTNAME = globalParameter::getHostName();
      
      //svcname
      $this -> COORDSVCNAME = globalParameter::getCoordPort();
      
      //random string
      $this -> CHANGEDPREFIX = globalParameter::getChangedPrefix();
      
      //ignoreNotExist
      if ( !isset( $this -> ignoreNotExist ) && empty( $this -> ignoreNotExist ) )
      { 
         $this -> ignoreNotExist = false; 
      }
      
      $this -> COMMDOMAINNAME = $this -> CHANGEDPREFIX ."_domain";
      $this -> COMMCSNAME = $this -> CHANGEDPREFIX ."_cs";
      $this -> COMMCLNAME = $this -> CHANGEDPREFIX ."_cl";
      
      $this -> CATALOG_GROUPNAME = "SYSCatalogGroup";
      $this -> COORD_GROUPNAME = "SYSCoord";
      $this -> SPARE_GROUPNAME = "SYSSpare";
   }   

   /* ***************************************
   *init parameter 
   *connect db
   **************************************** */ 
   public function __construct()
   {
      $this -> init();
      
      echo "\n---Begin to connect database.\n";
      $address = $this -> COORDHOSTNAME.':'.$this -> COORDSVCNAME ;
      $this -> db  = new SequoiaDB( $address );
      $this -> err = $this -> db -> getError();
      if( $this -> err['errno'] !== 0 )
      {
         echo "Failed to connect database, Errno: ". $this->err['errno'] ."\n";
         return false;
      }
   }   

   /* ***************************************
   *close db
   **************************************** */  
   function __destruct()
   {
      echo "\n---Begin to close database.\n";
      
      $this -> db -> close();
      $this -> err = $this -> db -> getError();
      if( $this -> err['errno'] !== 0 )
      {
         echo "Failed to close database, Errno: ". $this -> err['errno'] ."\n";
         return false;
      }
   }
 
   /* ***************************************
   *judge database mode is standalone
   **************************************** */  
   function commIsStandlone()
   {
      $this ->db -> list( SDB_LIST_GROUPS );
      $this -> err = $this -> db -> getError();
      if( $this -> err['errno'] === -159 ) //-159: The operation is for coord node only
      {
         echo "   Is standlone mode!! \n";
         return true;
      }
      return false;
   }
   
   /* ***************************************
   *connect catalog
   **************************************** */  
   function commConnectCatalog()
   {
		$rgObj = $this -> db -> getGroup( $this -> CATALOG_GROUPNAME );
      $this -> err = $this -> db -> getError();
      if( $this -> err['errno'] !== 0 )
      {
         echo "\nFailed to get cataRG. Errno: ". $this -> err['errno'] ."\n";
      }
      
		$nodeObj = $rgObj -> getMaster();
      $this -> err = $this -> db -> getError();
      if( $this -> err['errno'] !== 0 )
      {
         echo "\nFailed to get cataMaster. Errno: ". $this -> err['errno'] ."\n";
      }
		
		$cataAddr = $nodeObj -> getName();
      $cataDB = new SequoiaDB( $cataAddr );
      $this -> err = $cataDB -> getError();
      if( $this -> err['errno'] !== 0 )
      {
         echo "\nFailed to connect catalog. Errno: ". $this -> err['errno'] ."\n";
      }
		
      return $cataDB;
   }
   
   /* ***************************************
   *get dataRG Names
   **************************************** */  
   function commGetGroupNames()
   {
      $cursor = $this -> db -> list( SDB_LIST_GROUPS );
      $this -> err = $this -> db -> getError();
      if( $this -> err['errno'] !== 0 )
      {
         echo "\nFailed to get groups. Errno: ". $this -> err['errno'] ."\n";
      }
      
      while( $tmpInfo = $cursor -> next() )
      {
         $tmpName = $tmpInfo['GroupName'];
         if( $tmpName !== $this -> CATALOG_GROUPNAME && 
             $tmpName !== $this -> COORD_GROUPNAME   && 
             $tmpName !== $this -> SPARE_GROUPNAME )
         {
            array_push( $this -> groupNames, $tmpName );
         }
      }
      
      return $this -> groupNames;
   }
   
   /* ***************************************
   *create domain
   **************************************** */ 
   function commCreateDomain( $dmName, $options = null )
   {
      return $this -> db -> createDomain( $dmName, $options );
   }
   
   /* ***************************************
   *drop domain
   **************************************** */ 
   function commDropDomain( $dmName, $ignoreNotExist = false )
   {
      $this -> db -> dropDomain( $dmName ); 
      
      $this -> err = $this -> getErrno();
      if( $this -> err !== 0 && $this -> err !== -214 && $ignoreNotExist )
      {
         echo "\nFailed to drop domain. Errno: ". $this -> err ."\n";
      }
   }

   /* ***************************************
   *create CS
   **************************************** */ 
   function commCreateCS( $csName, $options = null )
   {
      return $this -> db -> createCS( $csName, $options );
   }
   
   /* ***************************************
   *drop CS
   **************************************** */ 
   function commDropCS( $csName, $ignoreNotExist = false )
   {
      $this -> db -> dropCS( $csName ); 
      
      $this -> err = $this -> getErrno();
      if( $this -> err !== 0 )
      {
         if( $this -> err === -34 && $ignoreNotExist )
         {
            //OK
         }
         else
         {
            echo "\nFailed to drop cs. \n";
            return $this -> err;
         }
      }
   }

   /* ***************************************
   *create CL
   **************************************** */ 
   function commCreateCL( $csName, $clName, $options = null, $autoCreateCS = false )
   {
      if( $autoCreateCS === true )
      {
         $this -> db -> selectCS( $csName );
         $this -> err = $this -> getErrno();
         if( $this -> err !== 0 )
         {
            throw new Exception( "Failed to select cs. Errno: ". $this -> err );
         }
      }
      
      $csDB = $this -> db -> getCS( $csName );
      $this -> err = $this -> getErrno();
      if( $this -> err !== 0 )
      {
         throw new Exception( "Failed to get cs. Errno: ". $this -> err );
      }
      
      $this -> err = $csDB -> createCL( $clName, $options );
      if( $this -> err['errno'] !== 0 )
      {
         throw new Exception( "Failed to create cl. Errno: ". $this -> err['errno'] );
      }
      return $csDB -> getCL( $clName ) ;
   }
   
   /* ***************************************
   *drop CL
   **************************************** */ 
   function commDropCL( $csName, $clName, $ignoreNotExist = false )
   {
      $csDB = $this -> db -> getCS( $csName );
      $this -> err = $this -> getErrno();
      if( $this -> err === 0 )
      {
         $csDB -> dropCL( $clName ); 
         $this -> err = $this -> getErrno();
         if( $this -> err !== 0 )
         {
            if( ( $this -> err === -23 && $ignoreNotExist ) ||
                ( $this -> err === -34 && $ignoreNotExist ) )
            {
               //continue;
            }
            else
            {
               echo "\nFailed to drop cl. \n";
               return $this -> err;
            }
         }
      }
      else if( $this -> err !== 0 && !$ignoreNotExist )
      {
         echo "\nFailed to get cs. \n";
         return $this -> err;
      }
   }

}
?>