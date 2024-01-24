/****************************************************************
@decription:   Data rebalance

@input:        mode:       String, "rebalance" or "shrink", required
               coord:      String, eg: "localhost:11810", required
    shrink input:
               hostlist:   Array, eg: ['host1','host2','host3']
               checkonly:  Boolean, check only, don't do shrink
    rebalance input:
               cl:         String, eg: "foo.bar"
               unit:       Number, balance unit, default: 100MB

@example:
    ../../bin/sdb -f dataRebalance.js -e 'var mode="rebalance";var coord="localhost:11810";var cl="foo.bar"'
    ../../bin/sdb -f dataRebalance.js -e 'var mode="shrink";var coord="localhost:11810";var hostlist=["host1","host2","host3"]'

@author:       Ting YU 2020-11-05
****************************************************************/

var MODE = "" ;
var COORD = "" ;
var BALANCEUNIT = 100 ; // MB
var CLFULLNAME = "" ;
var SHRINKHOSTLIST = [] ;
var CHECKONLY = false ;

// check parameter
if ( typeof( mode ) === "undefined" )
{
   throw new Error( "no parameter [mode] specified" ) ;
}
else if( mode.constructor !== String )
{
   throw new Error( "Invalid para[mode], should be String" ) ;
}
else if ( mode != "rebalance" && mode != "shrink" )
{
   throw new Error( "Invalid para[mode], should be 'rebalance' or 'shrink'" ) ;
}
MODE = mode ;

if ( typeof( coord ) === "undefined" )
{
   throw new Error( "no parameter [coord] specified" ) ;
}
else if( coord.constructor !== String )
{
   throw new Error( "Invalid para[coord], should be String" ) ;
}
COORD = coord ;

if ( mode == "rebalance" )
{
   if ( typeof( cl ) === "undefined" )
   {
      throw new Error( "no parameter [cl] specified" ) ;
   }
   else if( cl.constructor !== String )
   {
      throw new Error( "Invalid para[cl], should be String" ) ;
   }
   CLFULLNAME = cl ;

   if ( typeof( unit ) === "undefined" )
   {
      var unit = 100 ;
   }
   else if( unit.constructor !== Number )
   {
      throw new Error( "Invalid para[unit], should be Number" ) ;
   }
   else if ( unit <= 16 )
   {
      throw new Error( "Invalid para[unit], should be greate than 16" ) ;
   }
   BALANCEUNIT = unit ;
}
else
{
   if ( typeof( checkonly ) === "undefined" )
   {
      var checkonly = false ;
   }
   else if( checkonly.constructor !== Boolean )
   {
      throw new Error( "Invalid para[checkonly], should be Boolean" ) ;
   }
   CHECKONLY = checkonly ;

   if ( typeof( hostlist ) === "undefined" )
   {
      throw new Error( "no parameter [hostlist] specified" ) ;
   }
   else if( hostlist.constructor !== Array )
   {
      throw new Error( "Invalid para[hostlist], should be Array" ) ;
   }
   SHRINKHOSTLIST = hostlist ;
}

/*************** main entry *****************/
try
{
   var db = new Sdb( COORD ) ;
}
catch( e )
{
   printLog( "ERROR", "Failed to connect coord[" + e +
                      "], maybe coord[" + coord + "] is invalid" ) ;
   throw new Error() ;
}
try
{
   if ( MODE == "rebalance" )
   {
      rebalance() ;
   }
   else if ( MODE == "shrink" )
   {
      shrink() ;
   }
}
catch( e )
{
   if( e instanceof Error )
   {
      print( "Error Stack:\n" + e.stack );
   }
   throw e ;
}

function rebalance()
{
   printLog( "EVENT", "Data rebalance for cl[" + CLFULLNAME + "]" ) ;

   var remainGroupMap = new UtilMap() ;

   /// 1. check collection exist or not
   checkCLExist( CLFULLNAME ) ;

   /// 2. get remain groups by domain
   var domainName = getDomainByCL( CLFULLNAME ) ;
   if ( domainName == null )
   {
      printLog( "ERROR", "Collection[" + CLFULLNAME + "] doesn't belong " +
                         "to any domain" ) ;
      throw new Error() ;
   }

   getGroupsByDomain( domainName, remainGroupMap ) ;
   if ( remainGroupMap.size() == 0 )
   {
      printLog( "ERROR", "There are no remain groups" ) ;
      throw new Error() ;
   }

   /// 3. get sharding type
   var rc = db.snapshot( SDB_SNAP_CATALOG, { Name: CLFULLNAME } ) ;
   if ( !rc.next() )
   {
      printLog( "ERROR", "Collection[" + CLFULLNAME + "] doesn't exist" ) ;
      throw new Error() ;
   }
   var shardingType = rc.current().toObj().ShardingType ;

   /// 4. collection's data rebalance
   if ( shardingType == undefined )
   {
      printLog( "ERROR", "Collection[" + CLFULLNAME + "] is not sharding" ) ;
      throw new Error() ;
   }
   else if ( shardingType == "hash" )
   {
      rebalanceHashCL( CLFULLNAME, remainGroupMap ) ;
   }
   else if ( shardingType == "range" )
   {
      rebalanceRangeCL( CLFULLNAME, remainGroupMap ) ;
   }

   printLog( "EVENT", "Data rebalance done" ) ;
}

function shrink()
{
   printLog( "EVENT", "Data shrink for host[" + SHRINKHOSTLIST + "]" +
                      ( CHECKONLY ? ", only check" : "" ) ) ;

   var remainGroupList = new UtilList() ;
   var extraGroupList = new UtilList() ;

   /// 1. check and get groups by hostlist
   getGroupsByHostlist( remainGroupList, extraGroupList ) ;
   printLog( "EVENT", "Group at shrink host: ", extraGroupList ) ;

   if ( CHECKONLY )
   {
      return ;
   }

   /// 2. loop every collection
   var rc = db.snapshot( SDB_SNAP_CATALOG ) ;
   while ( rc.next() )
   {
      var rcObj = rc.current().toObj() ;
      var clFullName = rcObj.Name ;
      var catList = rcObj.CataInfo ;
      var remainGroupMap = new UtilMap() ;
      var extraGroupMap = new UtilMap() ;

      /// 2.1 get collection's remain groups and extra groups
      for ( var i in catList )
      {
         var groupName = catList[i].GroupName ;
         if ( extraGroupList.has( groupName ) )
         {
            if ( !extraGroupMap.has( groupName ) )
            {
               extraGroupMap.set( groupName, 0 ) ;
            }
         }
         else
         {
            if ( !remainGroupMap.has( groupName ) )
            {
               remainGroupMap.set( groupName, 0 ) ;
            }
         }
      }

      if ( extraGroupMap.size() == 0 )
      {
         printLog( "EVENT", "Collection[" + clFullName + "] isn't at " +
                            "shrinking host, no need to process it" ) ;
         continue ;
      }
      if ( remainGroupMap.size() == 0 )
      {
         var newGroupName = getNewGroup( clFullName, remainGroupList ) ;
         remainGroupMap.set( newGroupName, 0 ) ;
      }

      /// 2.2 data shrink and rebalance
      printLog( "EVENT", "Collection[" + clFullName + "] is at shrinking host" ) ;

      var type = rcObj.ShardingType ;
      if ( type == undefined )
      {
         var csName = clFullName.split( "." )[0] ;
         var clName = clFullName.split( "." )[1] ;
         var clHandle = db.getCS( csName ).getCL( clName ) ;

         clHandle.enableSharding( { ShardingKey: { _id: 1 } } ) ;

         rebalanceHashCL( clFullName, remainGroupMap ) ;

         clHandle.disableSharding() ;
      }
      else if ( type == "hash" )
      {
         rebalanceHashCL( clFullName, remainGroupMap ) ;
      }
      else if ( type == "range" )
      {
         rebalanceRangeCL( clFullName, remainGroupMap ) ;
      }
   }

   printLog( "EVENT", "Data shrink done" ) ;

}

function getGroupsByHostlist( remainGroup, extraGroup )
{
   var tmpHostList = [].concat( SHRINKHOSTLIST ) ;

   // loop every groups
   var rc = db.list( 7 ) ;
   while ( rc.next() )
   {
      var obj = rc.current().toObj() ;
      var groupName = obj.GroupName ;
      var nodeList = obj.Group ;

      var inShrinkHost = false ;
      var inOtherHost = false ;

      if ( groupName == "SYSCatalogGroup" || groupName == "SYSCoord" )
      {
         continue ;
      }

      // Nodes of this group should ALL locate on 'hostlist' or ALL NOT locate
      // on 'hostlist'
      for ( var i in nodeList )
      {
         var hostname = nodeList[i].HostName ;
         if ( -1 != SHRINKHOSTLIST.indexOf( hostname ) )
         {
            inShrinkHost = true ;
            tmpHostList.splice( tmpHostList.indexOf( hostname ), 1 ) ;
         }
         else
         {
            inOtherHost = true ;
         }
         if ( inShrinkHost && inOtherHost )
         {
            printLog( "ERROR",
                      "The hostlist[" + SHRINKHOSTLIST + "] is invalid, group["
                      + groupName + "]'s nodes some locate at them, " +
                      "while other don't" ) ;
            throw new Error() ;
         }
      }

      // If this group's all nodes ALL locate on 'hostlist', then this group
      // belongs to extraGroup( the group list to shrink )
      if ( inShrinkHost )
      {
         extraGroup.push( groupName ) ;
      }
      else if ( inOtherHost )
      {
         remainGroup.push( groupName ) ;
      }
   }

   // shrinking the hosts which don't exist in data groups is not allowed
   if ( tmpHostList.length > 0 )
   {
      printLog( "ERROR",
                "The hostlist[" + SHRINKHOSTLIST + "] is invalid, [" +
                tmpHostList + "] doesn't belong to this cluster's data groups" ) ;
      throw new Error() ;
   }

   // shrinking all hosts of data groups is not allowed
   if ( remainGroup.size() == 0 )
   {
      printLog( "ERROR",
                "The hostlist[" + SHRINKHOSTLIST + "] is invalid, " +
                "shrinking all hosts of data groups is not allowed" ) ;
      throw new Error() ;
   }
}

function checkCLExist( clName )
{
   var tmp = clName.split( "." ) ;
   var csName = tmp[0] ;
   var clShortName = tmp[1] ;

   if ( csName == null      || csName == "" ||
        clShortName == null || clShortName == "" )
   {
      printLog( "ERROR", "Collection name[" + clName + "] is invalid" ) ;
      throw new Error() ;
   }

   try
   {
      db.getCS( csName ).getCL( clShortName ) ;
   }
   catch( e )
   {
      if ( -34 == e )
      {
         printLog( "ERROR", "Collection space[" + csName + "] doesn't exist" ) ;
         throw new Error() ;
      }
      else if ( -23 == e )
      {
         printLog( "ERROR", "Collection[" + clName + "] doesn't exist" ) ;
         throw new Error() ;
      }
      else
      {
         throw new Error() ;
      }
   }
}

// @return domain name
function getDomainByCL( clName )
{
   var domainName = null ;

   var csName = clName.split( "." )[0] ;

   var catMaster = db.getRG( 'SYSCatalogGroup' ).getMaster() ;
   var cat = new Sdb( catMaster ) ;
   var rc = cat.SYSCAT.SYSCOLLECTIONSPACES.find( { Name: csName } ) ;
   if ( rc.next() )
   {
      domainName = rc.current().toObj().Domain ;
   }
   else
   {
      printLog( "ERROR", "Collection space[" + csName + "] doesn't exist" ) ;
      throw new Error() ;
   }

   return domainName ;
}

function getGroupsByDomain( domainName, groupMap )
{
   var rc = db.getDomain( domainName ).listGroups() ;
   if ( rc.next() )
   {
      var arr = rc.current().toObj().Groups ;
      for ( var i = 0 ; i < arr.length ; i++ )
      {
         groupMap.set( arr[i].GroupName, 0 ) ;
      }
   }
}

function getNewGroup( clName, remainGroupList )
{
   // Get one new group which doesn't locate on shrink hostlist.
   // If collection has domain, get new group from domain's group list,
   // otherwise get new group from cluster's remain group list.
   var newGroupName = null ;

   var domainName = getDomainByCL( clName ) ;
   if ( domainName == null )
   {
      newGroupName = remainGroupList.getOne() ;
   }
   else
   {
      var groupMap = new UtilMap() ;
      getGroupsByDomain( domainName, groupMap ) ;

      var domainGroups = groupMap.getKeys()
      for ( var i in domainGroups )
      {
         var g = domainGroups[i] ;
         if ( remainGroupList.has( g ) )
         {
            newGroupName = g ;
            break ;
         }
      }
      if ( newGroupName == null )
      {
         printLog( "ERROR", "Can't find any other groups to split data. " +
                            "Groups of collection[" + clName + "]'s domain " +
                            "all locate at shrink hostlist[" + SHRINKHOSTLIST +
                            "], you should add new groups to the domain[" +
                            domainName + "]" ) ;
         throw new Error() ;
      }
   }

   return newGroupName ;
}

function rebalanceRangeCL( clName, remainGroupMap )
{
   while ( true )
   {
      // 1. get every group's data size by snapshot
      var extraGroupMap = new UtilMap() ;
      getAllGroupsDataSize( clName, remainGroupMap, extraGroupMap ) ;
      if ( isDataBalanced( remainGroupMap, extraGroupMap ) )
      {
         printLog( "EVENT",
                   "Data of collection[" + clName + "] is balanced. " +
                   "group & datasize: ",
                   remainGroupMap ) ;
         break ;
      }

      // 2. calculate average data size
      var avgSize = getAverage( remainGroupMap, extraGroupMap ) ;

      // 3. split collection util data is balanced
      if ( extraGroupMap.size() > 0 )
      {
         splitCL( clName, false,
                  extraGroupMap, extraGroupMap.size() - 1, 0,
                  remainGroupMap, 0, avgSize ) ;
      }
      else
      {
         splitCL( clName, false,
                  remainGroupMap, remainGroupMap.size() - 1, avgSize,
                  remainGroupMap, 0, avgSize ) ;
      }
   }
}

function rebalanceHashCL( clName, remainGroupMap )
{
   while ( true )
   {
      // 1. get every group's partition by snapshot
      var extraGroupMap = new UtilMap() ;
      getAllGroupsPartition( clName, remainGroupMap, extraGroupMap ) ;
      if ( isPartitionBalanced( remainGroupMap, extraGroupMap ) )
      {
         printLog( "EVENT",
                   "Partition of collection[" + clName + "] is balanced. " +
                   "group & partition: ",
                   remainGroupMap ) ;
         break ;
      }

      // 2. calculate average partition
      var avgSize = getAverage( remainGroupMap, extraGroupMap ) ;

      // 3. split collection util partition is balanced
      if ( extraGroupMap.size() > 0 )
      {
         splitCL( clName, true,
                  extraGroupMap, extraGroupMap.size() - 1, 0,
                  remainGroupMap, 0, avgSize ) ;
      }
      else
      {
         splitCL( clName, true,
                  remainGroupMap, remainGroupMap.size() - 1, avgSize,
                  remainGroupMap, 0, avgSize ) ;
      }
   }
}

function getAllGroupsDataSize( clName, remainGroupMap, extraGroupMap )
{
   remainGroupMap.clearValues() ;
   extraGroupMap.clearValues() ;

   // snapshot collection return all group's all node's data size
   var rc = db.snapshot( SDB_SNAP_COLLECTIONS,
                         { RawData: true, Name: clName } ) ;
   while ( rc.next() )
   {
      var obj = rc.current().toObj() ;
      var groupName = obj.Details[0].GroupName ;
      var nodeName  = obj.Details[0].NodeName ;
      var pageSize  = obj.Details[0].PageSize ;
      var dataPages = obj.Details[0].TotalDataPages ;
      var freeSize  = obj.Details[0].TotalDataFreeSpace ;

      // check this node is master or not, we only use master node's information
      var master = db.getRG( groupName ).getMaster() ;
      if ( master == nodeName )
      {
         var dataSize = ( dataPages * pageSize - freeSize ) / 1048576 ; // MB

         // find out from 2 map, and set value(data size)
         var pos = remainGroupMap.pos( groupName ) ;
         if ( pos != -1 )
         {
            remainGroupMap.setByPos( pos, dataSize ) ;
         }
         else
         {
            pos = extraGroupMap.pos( groupName ) ;
            if ( pos != -1 )
            {
               extraGroupMap.setByPos( pos, dataSize ) ;
            }
            else
            {
               extraGroupMap.set( groupName, dataSize ) ;
            }
         }
      }
   }
}

function getAllGroupsPartition( clName, remainGroupMap, extraGroupMap )
{
   remainGroupMap.clearValues() ;
   extraGroupMap.clearValues() ;

   // snapshot catalog return all group's partition
   var rc = db.snapshot( SDB_SNAP_CATALOG, { Name: clName } ) ;
   if ( rc.next() )
   {
      var rcObj = rc.current().toObj() ;
      var partitionList = rcObj.CataInfo ;
      for ( var i in partitionList )
      {
         var obj = partitionList[i] ;
         var groupName = obj.GroupName ;
         var partition = obj.UpBound[""] - obj.LowBound[""] ;

         // find out from 2 map, and set value(partition)
         var pos = remainGroupMap.pos( groupName ) ;
         if ( pos != -1 )
         {
            var orgValue = remainGroupMap.get( groupName ) ;
            remainGroupMap.setByPos( pos, orgValue + partition ) ;
         }
         else
         {
            pos = extraGroupMap.pos( groupName ) ;
            if ( pos != -1 )
            {
               var orgValue = extraGroupMap.get( groupName ) ;
               extraGroupMap.setByPos( pos, orgValue + partition ) ;
            }
            else
            {
               extraGroupMap.set( groupName, partition ) ;
            }
         }
      }
   }
}

// @return bool
function isDataBalanced( remainGroupMap, extraGroupMap )
{
   remainGroupMap.sortByValue() ;

   // print debug log to trace bug
   //printLog( "DEBUG", "remain group & datasize: ", remainGroupMap ) ;
   //printLog( "DEBUG", "extra group & datasize: ", extraGroupMap ) ;

   var dataSizes = remainGroupMap.getValues() ;
   var maxSize = dataSizes[ dataSizes.length - 1 ] ;
   var minSize = dataSizes[ 0 ] ;

   if ( maxSize - minSize <= BALANCEUNIT && extraGroupMap.size() == 0 )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

// @return bool
function isPartitionBalanced( remainGroupMap, extraGroupMap )
{
   remainGroupMap.sortByValue() ;

   // print debug log to trace bug
   //printLog( "DEBUG", "remain group & partition: ", remainGroupMap ) ;
   //printLog( "DEBUG", "extra group & partition: ", extraGroupMap ) ;

   var partitions = remainGroupMap.getValues() ;
   var maxSize = partitions[ partitions.length - 1 ] ;
   var minSize = partitions[ 0 ] ;

   if ( maxSize - minSize <= 2 && extraGroupMap.size() == 0 )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

// @return avg size
function getAverage( remainGroupMap, extraGroupMap )
{
   var totalSize = 0 ;
   var groupCnt = remainGroupMap.size() ;

   var dataSize1 = remainGroupMap.getValues() ;
   for ( var i in dataSize1 )
   {
      totalSize += dataSize1[i] ;
   }

   var dataSize2 = extraGroupMap.getValues() ;
   for ( var i in dataSize2 )
   {
      totalSize += dataSize2[i] ;
   }

   var avgSize = totalSize / groupCnt ;

   return avgSize ;
}

function splitCL( clName, isHashCL,
                  srcGroupMap, srcPos, srcRemainValue,
                  tgtGroupMap, tgtPos, tgtRemainValue )
{
   var srcGroups = srcGroupMap.getKeys() ;
   var srcValues = srcGroupMap.getValues() ;

   var tgtGroups = tgtGroupMap.getKeys() ;
   var tgtValues = tgtGroupMap.getValues() ;

   var srcGroup = srcGroups[ srcPos ] ;
   var tgtGroup = tgtGroups[ tgtPos ] ;

   var srcSize = srcValues[ srcPos ] ;
   var tgtSize = tgtValues[ tgtPos ] ;

   // caculate the size to be splited
   var srcDeltaSize = srcSize - srcRemainValue ;
   var tgtDeltaSize = tgtRemainValue - tgtSize ;
   var deltaSize = Math.min( srcDeltaSize, tgtDeltaSize ) ;
   if ( isHashCL )
   {
      // partition is interger, NOT float
      deltaSize = Math.round( deltaSize ) ;
      deltaSize = deltaSize == 0 ? 1 : deltaSize ; // eg: 0.3 round => 0
   }
   else
   {
      // if delta is 0.x M, ceil to 1 MB
      deltaSize = deltaSize < 1 ? 1 : deltaSize ;
   }

   // caculate the percent to be splited
   if ( 0 == deltaSize )
   {
      printLog( "ERROR",
                "Collection[" + clName + "] split, delta size cann't be 0" ) ;
      throw new Error() ;
   }

   // range cl without data, its srouce size is 0
   var percent = srcSize == 0 ? 100 : deltaSize / srcSize * 100 ;
   percent = percent > 100 ? 100 : percent ;
   if ( percent > 99 && srcRemainValue == 0 )
   {
      // if split more than 99 percent, and remain data size / partition of
      // source group is expected to be 0, then just split 100 percent
      percent = 100 ;
   }

   // collection split
   var csName      = clName.split( "." )[0] ;
   var clShortName = clName.split( "." )[1] ;
   var collection  = db.getCS( csName ).getCL( clShortName ) ;

   printLog( "EVENT",
             "Begin to split collection[" + clName + "] from group[" +
             srcGroup + "] to group[" + tgtGroup + "] by percent[" + percent +
             "]" + " delta[" + deltaSize + "]" ) ;

   collection.split( srcGroup, tgtGroup, percent ) ;
}

function printLog( logLevel, log, utilClass )
{
   // eg: Timestamp() return: Timestamp("2020-11-10-17.06.23.769361")
   var time = Timestamp().toString().slice( 11, -9 ) ;

   print( time + " [" + logLevel + "]: " + log ) ;

   if ( typeof( utilClass ) !== "undefined" )
   {
      if ( utilClass.constructor === UtilMap )
      {
         utilClass.print() ;
      }
      else if ( utilClass.constructor === UtilList )
      {
         utilClass.print() ;
      }
   }

   println() ;
}

function UtilList()
{
   this._data = [] ;
   this.push = function( value )
   {
      this._data.push( value ) ;
   }
   this.has = function( value )
   {
      if ( -1 == this._data.indexOf( value ) )
      {
         return false ;
      }
      else
      {
         return true ;
      }
   }
   this.size = function()
   {
      return this._data.length ;
   }
   this.getOne = function()
   {
      var i = Math.floor( Math.random() * this._data.length ) ;
      return this._data[i] ;
   }
   this.print = function()
   {
      // print: [group1,group2,group3, ... ]
      print( "[" + this._data + "]" ) ;
   }
}

function UtilMap()
{
   this._data = [] ;
   this.set = function( key, value )
   {
      this._data.push( { Key: key, Value: value } ) ;
   }
   this.setByPos = function( pos, value )
   {
      if ( pos >= 0 && pos < this._data.length )
      {
         var curkey = this._data[pos].Key ;
         this._data[pos] = { Key: curkey, Value: value } ;
      }
   }
   this.get = function( key )
   {
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         if ( key == this._data[i].Key )
         {
            return this._data[i].Value ;
         }
      }
      return null ;
   }
   this.remove = function( key )
   {
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         if ( key == this._data[i].Key )
         {
            this._data.splice( i, 1 ) ;
            return ;
         }
      }
   }
   this.removeByPos = function( pos )
   {
      if ( pos >= 0 && pos < this._data.length )
      {
         this._data.splice( pos, 1 ) ;
      }
   }
   this.clear = function()
   {
      this._data = [] ;
   }
   this.clearValues = function()
   {
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         var curkey = this._data[i].Key ;
         this._data[i] = { Key: curkey, Value: 0 } ;
      }
   }
   this.size = function()
   {
      return this._data.length ;
   }
   this.pos = function( key )
   {
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         if ( key == this._data[i].Key )
         {
            return i ;
         }
      }
      return -1 ;
   }
   this.has = function( key )
   {
      return -1 != this.pos( key ) ;
   }
   this.getKeys = function( )
   {
      var arr = [] ;
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         arr.push( this._data[i].Key ) ;
      }
      return arr ;
   }
   this.getValues = function( )
   {
      var arr = [] ;
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         arr.push( this._data[i].Value ) ;
      }
      return arr ;
   }
   this.sortByKey = function()
   {
      this._data.sort( function( a, b ) { return a.Key - b.Key ; } ) ;
   }
   this.sortByValue = function()
   {
      this._data.sort( function( a, b ) { return a.Value - b.Value ; } ) ;
   }
   function _compareKey( a, b )
   {
      return a.Key - b.Key ;
   }
   function _compareValue( a, b )
   {
      return a.Value - b.Value ;
   }
   this.print = function()
   {
      // print: [group1:100,group2:0,group3:100,... ]
      print( "[" ) ;
      for( var i in this._data )
      {
         if ( i != 0 )
         {
            print( ',' ) ;
         }
         print( this._data[i].Key + ':' + this._data[i].Value ) ;
      }
      print( "]" ) ;
   }
}

