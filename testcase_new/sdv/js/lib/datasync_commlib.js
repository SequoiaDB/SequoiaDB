/*******************************************************************************
*@Description: datasync commlib
*@Modify list:
*   2015-10-12 wenjing wang  Init
*******************************************************************************/
var w = {
   ALLACTIVE: -1, ALL: 0, ONE: 1, TWO: 2,
   THREE: 3, FOUR: 4, FIVE: 5, SIX: 6, SERVER: 7
};
var chkStep = { FIRST: 1, SECOND: 2, THIRD: 3, FOURTH: 4, FIFTH: 5 };

// build random string
function buildStr ( len )
{
   if( "undefined" === typeof ( len ) )
   {
      var len = 10;
   }

   var str = "";
   var characters = new Array( 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
      'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
      'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
      'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9' );

   for( var i = 0; i < len; ++i )
   {
      var pos = Math.floor( Math.random() * characters.length );
      str += characters[pos];
   }

   return str;
}

function isNeedSleep ( replSize )
{
   if( "undefined" === typeof ( replSize ) )
   {
      var replSize = 0;
   }

   var needSleep = true;
   if( w.ALL === replSize
      || w.SERVER === replSize
      || w.ALLACTIVE === replSize
      || nodes.length === replSize )
   {
      needSleep = false;
   }
   return needSleep;
}

function buildCond ( obj, name )
{
   var cond = {}
   cond.Name = name;
   for( elem in obj )
   {
      cond[elem] = obj[elem];
   }

   return cond;
}

// collection object
function collection ( csName, clName, replSize )
{
   var funname = "collection";
   if( "undefined" === typeof ( csName ) )
   {
      throw new Error("csName is undefined") ;
   }

   if( "undefined" === typeof ( clName ) )
   {
      throw new Error("clName is undefined") ;
   }

   if( "undefined" === typeof ( replSize ) )
   {
      throw new Error("replSize is undefined") ;
   }

   this.csName = csName;
   this.clName = clName;
   this.replSize = replSize;
}

collection.prototype.create =
   function( db, groupName )
   {
      if( "undefined" === typeof ( db ) )
      {
         throw new Error("db is undefined") 
      }

      if( "undefined" === typeof ( groupName ) )
      {
         throw new Error("groupName is undefined") ;
      }

      this.groupName = groupName;
      println( "groupName" + this.groupName );
      var option = { ReplSize: this.replSize, Group: this.groupName };
      println( "createCL's option:" + JSON.stringify( option ) );
      this.cl = commCreateCL( db, this.csName, this.clName, option, true, true );
   }

collection.prototype.getSelf =
   function()
   {
      return this.cl;
   }

collection.prototype.drop =
   function( db )
   {
      if( undefined === db )
      {
         throw new Error("db is undefined");
      }
      commDropCL( db, this.csName, this.clName );
   }

collection.prototype.insert =
   function( docSize )
   {
      if( "number" !== typeof ( docSize ) )
      {
         throw new Error("invalid parameter") ;
      }

      try
      {
         var doc = { _id: 1 };
         // total number of bytes(4) + element type(1) + key(_id/4) + value(4)
         doc.str = buildStr( docSize - 13 );

         this.cl.insert( doc );
         this.condition = {};
         this.operator = "insert";
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.bulkInsert =
   function( number )
   {
      if( "number" !== typeof ( number ) )
      {
         throw new Error("invalid parameter") ;
      }
      try
      {
         var batchSize = 2000;
         var cnt = 0;
         for( var k = 0; k < number; k += batchSize )
         {
            batchSize = batchSize * ( cnt + 1 ) < number ? batchSize : number - batchSize * cnt;
            var objs = [];
            for( var i = 0; i < batchSize; ++i )
            {
               var len = i;
               var baseVal = batchSize * cnt;
               if( len > 512 )
               {
                  len = 512;
               }
               var ostr = buildStr( len );
               var obj = { _id: i + baseVal, a: i + baseVal, b: i + baseVal, str: ostr };
               objs.push( obj );
            }

            this.cl.insert( objs );
            cnt++;
         }
         this.condition = {};
         this.operator = "bulkinsert";
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.update =
   function( condition )
   {
      if( "object" !== typeof ( condition ) )
      {
         throw new Error("invalid parameter") ;
      }

      try
      {
         this.cl.update( { $set: { str: "test datasync" } }, condition );
         this.condition = condition;
         this.operator = "update";
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.delete =
   function( condition )
   {
      if( "object" !== typeof ( condition ) )
      {
         throw new Error("invalid parameter") ;
      }

      try
      {
         this.cl.delete( condition );
         this.condition = condition;
         this.operator = "delete";
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.removeAll =
   function()
   {
      try
      {
         this.cl.remove();
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.createIndex =
   function( name, indexDef )
   {
      if( undefined === name )
      {
         var name = 'idxtest';
      }

      if( "object" !== typeof ( indexDef ) )
      {
         throw new Error("indexDef is not object") ;
      }

      try
      {
         this.cl.createIndex( name, indexDef );
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.dropIndex =
   function( idxName )
   {
      if( "string" !== typeof ( idxName ) )
      {
         throw new Error("idxName must is string") ;
      }

      try
      {
         this.cl.dropIndex( idxName );
      }
      catch( e )
      {
         throw new Error(e) ;
      }
   }


collection.prototype.find =
   function( cond )
   {

   }

collection.prototype.explain =
   function( db, cond )
   {
      if( "object" !== typeof ( db ) )
      {
         throw new Error("invalid parameter") ;
      }

      if( "undefined" === typeof ( cond ) )
      {
         cond = {};
      }

      try
      {
         var cl = db.getCS( this.csName ).getCL( this.clName );
         var res = [];
         var obj = cl.find( cond ).explain().next().toObj();

         res.push( obj["ScanType"] )
         if( "ixscan" === obj["ScanType"] )
         {
            res.push( obj["IndexName"] );
         }
         return res;
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.putLob =
   function( fileName )
   {
      try
      {
         this.lobId = this.cl.putLob( fileName );
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.getLob =
   function( cl )
   {
      try
      {
         if( undefined !== cl )
         {
            cl.getLob( this.lobId, "/tmp/testLob" );
         }
         else
         {
            this.cl.getLob( this.lobId, "/tmp/testLob" );
         }
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.getCount =
   function( db, cond )
   {
      if( "object" !== typeof ( db ) )
      {
         throw new Error("invalid parameter") ;
      }

      if( "undefined" === typeof ( cond ) )
      {
         cond = {};
      }

      try
      {
         var cl = db.getCS( this.csName ).getCL( this.clName );
         this.count = cl.count( cond );
         return this.count;
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

collection.prototype.checkCountAfterOperate =
   function( db )
   {
      if( "object" !== typeof ( db ) )
      {
         throw new Error("invalid parameter") ;
      }

      try
      {
         var cl = db.getCS( this.csName ).getCL( this.clName );
         if( "undefined" === typeof ( this.condition ) )
         {
            return true;
         }

         var count = cl.count( this.condition );
         if( "delete" === this.operator )
         {
            return count === 0 ? true : false;
         }
         else if( "update" === this.operator )
         {
            return count === 1 ? true : false;
         }
      }
      catch( e )
      {
         throw new Error(e);
      }

      return true;
   }

function replicaNode ( hostName, svcName, group )
{
   if( "undefined" === typeof ( hostName ) )
   {
      throw new Error("hostName is undefined") ;
   }

   if( "undefined" === typeof ( svcName ) )
   {
      throw new Error("svcName is undefined") ;
   }

   if( "undefined" === typeof ( group ) )
   {
      throw new Error("group is undefined");
   }

   this.hostName = hostName;
   this.svcName = svcName;
   this.group = group;
   this.dbPath = "";
   this.config = { weight: 100 };
}

replicaNode.prototype.setDbPath =
   function( path )
   {
      if( "string" !== typeof ( path ) )
      {
         throw new Error("invalid parameter") ;
      }

      this.dbPath = path;
   }

replicaNode.prototype.setConfig =
   function( config )
   {
      if( "object" !== typeof ( config ) )
      {
         throw new Error("invalid parameter") ;
      }

      this.config = config;
   }

replicaNode.prototype.create =
   function()
   {
      try
      {
         var originalGroup = this.group.getSelf();
         originalGroup.createNode( this.hostName, this.svcName, this.dbPath, this.config );
         originalGroup.start();
         this.group.addNode( this );
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

replicaNode.prototype.drop =
   function()
   {
      try
      {
         var originalGroup = this.group.getSelf();
         originalGroup.removeNode( this.hostName, this.svcName );
         this.group.delNode( this );
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

replicaNode.prototype.connect =
   function()
   {
      var isOk = true;
      do
      {
         try
         {
            println( "connect " + this.hostName + ":" + this.svcName );
            this.db = new Sdb( this.hostName, this.svcName );
            isOk = true;
         }
         catch( e )
         {
            if( -104 === e )
            {
               isOk = false;
               continue;
            }
            throw new Error(e);
         }
      } while( !isOk );
   }

replicaNode.prototype.disConnect =
   function()
   {
      try
      {
         if( "undefined" !== typeof ( this.db ) )
         {
            this.db.close();
         }
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

replicaNode.prototype.getCurrentLsn =
   function()
   {
      try
      {
         if( "undefined" === typeof ( this.db ) )
         {
            this.connect();
         }

         var snapshot6 = this.db.snapshot( 6 ).toArray();
         var obj = eval( "(" + snapshot6 + ")" );
         this.currentLsn = obj.CurrentLSN.Offset;

         return this.currentLsn;
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

replicaNode.prototype.toString =
   function()
   {
      return this.hostName + ":" + this.svcName;
   }

replicaNode.prototype.getAllCS =
   function()
   {
      var csSet = [];
      try
      {
         if( "undefined" === typeof ( this.db ) )
         {
            this.connect();
         }

         var cursor = this.db.list( 5 );
         while( cursor.next() )
         {
            var obj = cursor.current().toObj();
            //适配mvcc分支，RBS集合目前实现，只写主节点不写备节点，该集合空间不对主备一致性进行校验
            if(obj.Name !== "SYSRBS")
            {
               csSet.push( obj.Name );
            }
         }
      }
      catch( e )
      {
         throw new Error(e);
      }

      return csSet;
   }

replicaNode.prototype.getAllCL =
   function()
   {
      var clSet = [];
      try
      {
         if( "undefined" === typeof ( this.db ) )
         {
            this.connect();
         }

         var cursor = this.db.list( 4 );
         while( cursor.next() )
         {
            var obj = cursor.current().toObj();
            //mvcc分支，RBS集合目前实现，只写主节点不写备节点，该集合空间下的集合不对主备一致性进行校验
            if(obj.Name.split(".")[0] !== "SYSRBS")
            {
               clSet.push( obj.Name );
            }
         }
      }
      catch( e )
      {
         throw new Error(e);
      }

      return clSet;
   }

function groupMgr ( db )
{
   if( "undefined" === typeof ( db ) )
   {
      throw new Error("invalid parameter");
   }

   this.db = db;
   this.groupSet = [];
}

groupMgr.prototype.init =
   function()
   {
      try
      {
         var groups = commGetGroups( this.db, false, "", false, false, false );
         for( var i = 0; i < groups.length; ++i )
         {
            var group = new replicaGroup( this.db, groups[i][0].GroupName, groups[i][0].GroupID );
            println( groups[i][0].GroupName + " begin" );
            for( var j = 1; j < groups[i].length; ++j )
            {
               var node = new replicaNode( groups[i][j].HostName, groups[i][j].svcname, group );
               node.setDbPath( groups[i][j].dbpath );
               println( "node: " + groups[i][j].HostName + ":" + groups[i][j].svcname );
               group.addNode( node );
            }
            println( groups[i][0].GroupName + " end" );

            this.groupSet.push( group );
         }
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

groupMgr.prototype.size =
   function()
   {
      return this.groupSet.length;
   }

groupMgr.prototype.getGroupByName =
   function( name )
   {
      if( "string" !== typeof ( name ) )    
      {
         throw new Error("invalid parameter");
      }

      for( var i = 0; i < this.groupSet.length; ++i )
      {
         if( this.groupSet[i].name === name )
         {
            return this.groupSet[i];
         }
      }

      throw  new Error( "the group " + name + "is not exist") ;
   }

groupMgr.prototype.getGroupByPos =
   function( index )
   {
      if( index >= 0 && index <= this.size() )
      {
         return this.groupSet[index];
      }
      else
      {
         throw new Error( "groups[" + index + "] is not exist") ;
      }
   }


// ReplicaGroup
function replicaGroup ( db, name, id )
{
   var funname = "replicaGroup";
   if( "undefined" === typeof ( db ) )
   {
      throw new Error("db is undefined") ;
   }

   if( "undefined" === typeof ( name ) )
   {
      throw new Error("name is undefined") ;
   }

   if( "undefined" === typeof ( id ) )
   {
      throw new Error("id is undefined") ;
   }

   this.db = db;
   this.name = name;
   this.id = id;
   this.nodeSet = [];
   this.totalSleepDuration = 30000 ; 
}

replicaGroup.prototype.getSelf =
   function()
   {
      return this.db.getRG( this.name );
   }

replicaGroup.prototype.addNode =
   function( node )
   {
      if( "object" !== typeof ( node ) )
      {
         throw new Error("invalid parameter") ;
      }

      this.nodeSet.push( node );
   }

replicaGroup.prototype.delNode =
   function( node )
   {
      if( "object" !== typeof ( node ) )
      {
         throw new Error("invalid parameter") ;
      }

      var tmpnode = this.nodeSet.pop();
      if( tmpnode.hostName === node.hostName &&
         tmpnode.svcName === node.svcName )
      {
         return;
      }

      for( var i = 0; i < this.nodeSet.length; ++i )
      {
         if( tmpnode.hostName === node.hostName &&
            tmpnode.svcName === node.svcName )
         {
            break;
         }
      }

      if( i === this.nodeSet.length ) return;
      this.nodeSet.splice( i, 1 );
   }

replicaGroup.prototype.create =
   function()
   {
      try
      {
         switch( this.name )
         {
            case SPARE_GROUPNAME:
               this.db.createSpareRG();
               break;
            case COORD_GROUPNAME:
               this.db.createCoordRG();
               break;
            case CATALOG_GROUPNAME:
               break;
            default:
               this.db.createRG( this.name );
               break;
         }
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

replicaGroup.prototype.getSequoiadb =
   function( )
   {
	   return this.db ;
   }

replicaGroup.prototype.getName =
   function()
   {
      return this.name ;
   }

replicaGroup.prototype.drop =
   function()
   {
      try
      {
         this.db.removeRG( this.name );
      }
      catch( e )
      {
         throw new Error(e);
      }
   }

replicaGroup.prototype.size =
   function()
   {
      return this.nodeSet.length;
   }

replicaGroup.prototype.getNodeByPos =
   function( index )
   {
      if( index >= 0 && index <= this.size() )
      {
         return this.nodeSet[index];
      }
      else
      {
         throw new Error("nodeSet[" + index + "] is not exist") ;
      }
   }

replicaGroup.prototype.checkResult =
   function( needSleep, checkfun, cl, condition )
   {
      if( "function" !== typeof ( checkfun ) )
      {
         throw  new Error("checkfun is not function") ;
      }

      var totalSleepDuration = 120000;
      var sleepInteval = 10000;

      var sleepDuration = 0;
      var checkOk = true;

      do
      {
         if( needSleep )
         {
            sleep( sleepInteval );
            sleepDuration += sleepInteval;
            if ( sleepInteval > 1000 )
            {
               sleepInteval /= 2 ;
            }
         }
         else
         {
            sleepDuration = this.totalSleepDuration;
         }

         if( "undefined" !== typeof ( cl ) )
         {
            checkOk = checkfun( this, cl, condition );
         }
         else
         {
            checkOk = checkfun( this );
         }

      } while( !checkOk && sleepDuration < totalSleepDuration );

      return checkOk;
   }

replicaGroup.prototype.checkLSN =
   function( group )
   {
      var db = group.getSequoiadb();
      var snapshotRes = db.snapshot(6, {RawData:1,"GroupName":group.getName()},{NodeName:"", CompleteLSN:"", CurrentLSN:"",IsPrimary:""},{IsPrimary:-1}) ;
	  
      var prevLsn = 0;
      var prevSvc = "" ;
      if ( typeof(this.failedCound) === "undefined" )
      {
         this.failedCound = 0;
      }
      while ( snapshotRes.next() )
      {
         var obj = snapshotRes.current().toObj() ;
         if ( prevSvc === "" )
         {
            prevLsn = obj.CompleteLSN;
            prevSvc = obj.NodeName;
         }
         else if( prevLsn !== obj.CompleteLSN )
         {
            if ( this.failedCound++ == 0 ) 
            {
               println( prevSvc + "is LSN: " + prevLsn + obj.NodeName + " is LSN:" + obj.CompleteLSN );
            }
            return false;
         }
      }
      
      this.failedCound = 0 ;
      return true;
   }

replicaGroup.prototype.checkExplain =
   function( group, coll, cond )
   {
      if( "object" !== typeof ( coll ) )
      {
         throw new Error("invalid parameter") ;
      }

      var prevExplain = [];
      for( var i = 0; i < group.size(); ++i )
      {
         var node = group.getNodeByPos( i );
         node.connect();
         println( JSON.stringify( cond ) );
         var currentExplain = coll.explain( node.db, cond );
         if( 0 === i )
         {
            prevExplain = currentExplain;
         }
         else if( prevExplain.length !== currentExplain.length ||
            prevExplain[0] !== currentExplain[0] ||
            prevExplain[1] !== currentExplain[1] )
         {
            println( "*** " + node.toString() + JSON.stringify( currentExplain ) + JSON.stringify( prevExplain ) );
            return false;
         }
      }
      return true;
   }

replicaGroup.prototype.checkDoc =
   function( group, coll, cond )
   {
      if( "object" !== typeof ( coll ) )
      {
         throw new Error("invalid parameter") ;
      }

      var prevCnt = 0;
      for( var i = 0; i < group.size(); ++i )
      {
         var node = group.getNodeByPos( i );
         node.connect();
         println( JSON.stringify( cond ) );
         var totalSleepLen = 10000;
         var alreadySleep = 0;
         do
         {
            try
            {
               var currentCnt = coll.getCount( node.db, cond );
               break;
            } catch( e )
            {
               if( -23 != e.message )
               {
                  sleep( 10 );
                  alreadySleep += 10;

                  if( alreadySleep >= totalSleepLen )
                  {
                     throw new Error(e);
                  }
                  continue;
               }
			   throw new Error(e);
            }

            if( 0 === i )
            {
               prevCnt = parseInt( currentCnt );
            }
            else if( prevCnt !== parseInt( currentCnt ) )
            {
               if( alreadySleep < totalSleepLen )
               {
                  sleep( 10 );
                  alreadySleep += 10;
                  continue;
               }
               println( "*** " + node.toString() + " curr val=" + currentCnt + "prev val=" + prevCnt );
               return false;
            }

         } while( true )

         if( !coll.checkCountAfterOperate( node.db ) )
         {
            return false;
         }
      }

      return true;
   }

replicaGroup.prototype.checkConsistency =
   function( coll )
   {

      var totalTimeLen = 300000;
      var sleepTimeLen = 0;
      while( sleepTimeLen < totalTimeLen )
      {
         if( CATALOG_GROUPNAME === this.name ||
            COORD_GROUPNAME === this.name ||
            SPARE_GROUPNAME === this.name )
         {
            return true;
         }

         try
         {
            var installPath = commGetInstallPath();
            var cmd = new command( installPath + "/bin/sdbinspect" );
            cmd.addOption( "-g " + this.name );
            cmd.addOption( "-d " + this.db.toString() );
            cmd.addOption( "-c " + coll.csName );
            cmd.addOption( "-l " + coll.clName );

            var result = cmd.exec();
            println( result );
            if( result.lastIndexOf( "inspect done" ) === 0 &&
               result.lastIndexOf( "exit with no records different" ) !== -1 &&
               result.lastIndexOf("Total different collections count : 0") !== -1)
            {
               return true;
            }
            else
            {
               println( "sdbinspect exec result:" + result )
               sleep( 1000 );
               sleepTimeLen += 1000;
               continue;
            }
         }
         catch( e )
         {
            sleep( 1000 );
            sleepTimeLen += 1000;
            if( sleepTimeLen >= totalTimeLen )
               throw new Error(e);
            continue;
         }
      }//while  
      return false;
   }

replicaGroup.prototype.checkCS =
   function( group )
   {
      var pervCSSet = []
      for( var i = 0; i < group.size(); ++i )
      {
         var curCSSet = group.getNodeByPos( i ).getAllCS();
         if( 0 === i )
         {
            pervCSSet = curCSSet;
         }
         else if( !isArrEqual( pervCSSet, curCSSet ) )
         {
            println( JSON.stringify( pervCSSet ) );
            println( JSON.stringify( curCSSet ) );
            return false;
         }
      }

      return true;
   }

replicaGroup.prototype.checkCL =
   function( group )
   {
      var pervCLSet = []
      for( var i = 0; i < group.size(); ++i )
      {
         var curCLSet = group.getNodeByPos( i ).getAllCL();
         if( 0 === i )
         {
            pervCLSet = curCLSet;
         }
         else if( !isArrEqual( pervCLSet, curCLSet ) )
         {
            println( JSON.stringify( pervCLSet ) );
            println( JSON.stringify( curCLSet ) );
            return false;
         }
      }

      return true;
   }

function isExistInArray ( arr, elem )
{
   for( var i = 0; i < arr.length; ++i )
   {
      if( elem === arr[i] )
      {
         return true;
      }
   }
   return false;
}

function isArrEqual ( srcArr, destArr )
{
   if( srcArr.length !== destArr.length )
   {
      return false;
   }

   for( var k = 0; k < srcArr.length; ++k )
   {
      if( !isExistInArray( destArr, srcArr[k] ) )
      {
         return false;
      }
   }
   return true;
}

// the group's size is greater than or equal nodeNum
function selectGroupByNodeNum ( groupMgr, nodeNum )
{
   var groups = [];
   for( var i = 0; i < groupMgr.size(); ++i )
   {
      var group = groupMgr.getGroupByPos( i );
      if( CATALOG_GROUPNAME === group.name ||
         COORD_GROUPNAME === group.name ||
         SPARE_GROUPNAME === group.name )
      {
         continue;
      }

      if( group.size() >= nodeNum )
      {
         groups.push( group );
      }
   }

   if( 0 === groups.length )
   {
      println( "all data group node num less than " + nodeNum );
      return;
   }

   var pos = Math.floor( Math.random() * groups.length );
   println( groups[pos].name + " size is " + groups[pos].size() );
   return groups[pos];
}

function checkResult ( arrSdb, obj )
{
   if( undefined === arrSdb ||
      "array" !== arrSdb.constructor )
   {
      throw new Error("parameter arrSdb invalid") ;
   }

   if( undefined === obj ||
      "object" !== typeof ( obj ) )
   {
      throw new Error("parameter obj invalid") ;
   }

   this.totalSleepDuration = 10;
   this.sleepInteval = 2;
   this.arrSdb = arrSdb;

   checkResult.prototype.check =
      function()
      {
         var sleepDuration = 0;
         var checkOk = true;
         do
         {
            if( obj.needSleep() )
            {
               sleep( this.sleepInteval )
               sleepDuration += this.sleepInteval;
            }
            else
            {
               sleepDuration = this.totalSleepDuration;
            }

            for( var i = 0; i < this.arrSdb.length; ++i )
            {
               checkOk = obj.check( this.arrSdb[i] );
               if( !checkOk )
               {
                  break;
               }
            }
         } while( !checkOk && sleepDuration < totalSleepDuration );

         if( !checkOk )
         {
            obj.report();
         }
      }
}

// random select replsize
function selectReplSize ( nodeNumber )
{
   if( "undefined" === typeof ( nodeNumber ) )
   {
      return 0;
   }

   var replSize = Math.floor( Math.random() * w );
   if( w.FOUR == replSize )
   {
      replSize = w.SERVER;
   }
   else if( w.Five == replSize )
   {
      replSize = w.ALLACTIVE;
   }
   else if( replSize > nodeNumber )
   {
      replSize = nodeNumber;
   }
   return replSize;
}

function command ( name )
{
   if( "undefined" === typeof ( name ) )
   {
      throw new Error("name undefined") ;
   }

   this.name = name;
   this.cmd = new Cmd();
}

command.prototype.exec =
   function( newcmdstr )
   {
      try
      {
         if( "undefined" !== typeof ( newcmdstr ) )
         {
            var cmdstr = newcmdstr;
         }
         else
         {
            var cmdstr = "undefined" !== typeof ( this.options ) ?
               this.name + " " + this.options : this.name;
         }
         println( cmdstr );
         var result = this.cmd.run( cmdstr );
      }
      catch( e )
      {
         var exceptionMsg = "exec " + cmdstr + e;
         throw new Error(exceptionMsg) ;
      }

      return result;
   }

command.prototype.addOption =
   function( option )
   {
      if( "undefined" === typeof ( option ) )
      {
         throw new Error("option is undefined") ;
      }

      if( "undefined" === typeof ( this.options ) )
      {
         this.options = option;
      }
      else
      {
         this.options = this.options + " " + option;
      }
   }

function getHostNameOfLocal ()
{
   var cmd = new command( "hostname" );

   var hostName = cmd.exec();

   var endPos = hostName.length - 1;
   if( endPos === hostName.indexOf( '\n', 0 ) )
   {
      hostName = hostName.substr( 0, endPos );
   }

   return hostName;
}

function isPortUsed ( port )
{
   try
   {
      var cmd = new command( "lsof " );
      cmd.addOption( "-i:" + port );
      cmd.addOption( "|grep LISTEN" );
      cmd.exec();
      return true;
   }
   catch( e )
   {
      return false;
   }
}

// only valid at the host that is run script 
function allocPort ()
{
   var port = parseInt( RSRVPORTBEGIN );
   for( ; port < parseInt( RSRVPORTEND ); port += 10 )
   {
      if( !isPortUsed( port ) )
      {
         break;
      }
   }

   return port.toString();
}

function buildDeployPath ( port, groupType )
{
   var path = commGetInstallPath();

   path += "/"
   path += groupType;
   path += "/";
   path += port;

   return path;
}

function testFile ( filePath, fileName )
{
   this.filePath = filePath;
   this.fileName = fileName;
}

testFile.prototype.generator =
   function( fileSize )
   {

      try
      {
         println( "enter" );
         var fullPath = this.filePath + "/" + this.fileName;
         var file = new File( fullPath );
         var step = fileSize > 1024 ? 1024 : fileSize;
         for( var i = 0; i < fileSize; i += 1024 )
         {
            var str = buildStr( 1024 );
            file.write( str );
         }
      }
      catch( e )
      {
         throw new Error(e) ;
      }
      finally
      {
         file.close();
      }
   }

testFile.prototype.delete =
   function()
   {
      var cmd = new command( "rm -f " );
      var fullPath = this.filePath + "/" + this.fileName;
      cmd.addOption( fullPath );
      cmd.exec();
   }

testFile.prototype.getFileMd5 =
   function()
   {
      var cmd = new command( "md5sum" );
      cmd.addOption( this.fileName );
      var res = cmd.exec();
      var arrRes = res.split( " " );
      if( 2 === arrRes.length &&
         arrRes[1] === this.fileName )
      {
         return arrRes[0];
      }
      else
      {
         return "unknown";
      }
   }
