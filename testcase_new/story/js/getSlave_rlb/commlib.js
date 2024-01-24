import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/*******************************************************************
* @Description : check path has / in the end or not
*                add / if not
* @author      : Liang XueWang
********************************************************************/
function adaptPath ( path )
{
   if( path.lastIndexOf( '/' ) !== path.length - 1 )
      path += '/';
   return path;
}

/*******************************************************************
* @Description : create nodes in rg
*                hostname : local hostname, 
*                svcname  : RSRVPORTBEGIN ....
*                dbpath   : RSRVNODEDIR + "data/" + svcname
* @author      : Liang XueWang
*
********************************************************************/
function createNodes ( rg, nodeNum )
{
   var logSourcePaths = [];
   for( var i = 0; i < nodeNum; i++ )
   {
      var host = System.getHostName();
      var svc = parseInt( RSRVPORTBEGIN );
      var dbpath = adaptPath( RSRVNODEDIR ) + "data/" + svc;
      var checkSucc = false;
      var times = 0;
      var maxRetryTimes = 10;
      do
      {
         try
         {
            rg.createNode( host, svc, dbpath, { diaglevel: 5 } );
            checkSucc = true;
            logSourcePaths.push( host + ":" + CMSVCNAME + "@" + dbpath + "/diaglog/sdbdiag.log" );
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e.message == SDBCM_NODE_EXISTED || e.message == SDB_DIR_NOT_EMPTY )
            {
               svc = svc + 10;
               dbpath = adaptPath( RSRVNODEDIR ) + "data/" + svc;
            }
            else
            {
               throw e;
            }
            times++;
         }
      } while( !checkSucc && times < maxRetryTimes );
   }
   rg.start();
   return logSourcePaths;
}

/**********************************************************************
 * @Description : get nodes in group
 *                rgName: group name, ex "group1"
 *                return nodes array, ex [ "sdbserver1:20100", .... ]
 * @author      : Liang XueWang
 *
 **********************************************************************/
function getGroupNodes ( db, rgName )
{
   var arr = new Array();
   var tmpObj = db.getRG( rgName ).getDetail().next().toObj();
   var tmpGroupArray = tmpObj["Group"];
   for( var j = 0; j < tmpGroupArray.length; ++j )
   {
      var tmpNodeObj = tmpGroupArray[j];
      var hostName = tmpNodeObj["HostName"];
      for( var k = 0; k < tmpNodeObj.Service.length; ++k )
      {
         var tmpSvcObj = tmpNodeObj.Service[k];
         if( tmpSvcObj["Type"] == 0 )
         {
            nodeName = hostName + ":" + tmpSvcObj["Name"];
            arr.push( nodeName );
            break;
         }
      }
   }

   return arr;
}

/**********************************************************************
 * @Description : check group has master or not
 *                
 * @author      : Liang XueWang
 *
 **********************************************************************/
function isMasterExist ( db, rgName )
{
   var clName = "testHasMasterCl";
   var hasMaster = false;
   try
   {
      db.getCS( COMMCSNAME ).createCL( clName, { Group: rgName } );
      hasMaster = true;
      commDropCL( db, COMMCSNAME, clName );
   }
   catch( e )
   {
      if( e.message != SDB_CLS_NOT_PRIMARY )
      {
         throw e;
      }
   }
   return hasMaster;
}

/* ****************************************************
@description: get the primary node in the group
@author : wangkexin
@parameter: 
    rg : the specified group
@return: the primary node in the group
**************************************************** */
function getMaster ( rg )
{
   var master;
   while( true )
   {
      try
      {
         master = rg.getMaster();
         break;
      }
      catch( e )
      {
         if( e.message == SDB_RTN_NO_PRIMARY_FOUND )
         {
            continue;
         }
         else
         {
            throw e;
         }
      }
   }
   return master;
}