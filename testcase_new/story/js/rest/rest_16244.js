/******************************************************************************
 * @Description : test snapshot parameters: hint/limit/skip
 *                seqDB-16244:指定hint/limit/skip获取SDB_SNAP_DATABASE快照信息
 * @author      : Fan YU
 * @date        ：2018.10.23
 ******************************************************************************/



main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //get a group in cluster
   var groupName = commGetGroups( db )[0][0].GroupName;
   var nodeNum = commGetGroupNodes( db, groupName ).length;
   //the options of snapshot 
   var type = "database";
   var filter = "{RawData:true,GroupName:\"" + groupName + "\"}";
   var selector = "{ NodeName:" + null + ", GroupName:" + null + "}";
   var hint = "{$options:{expand:true}}";
   var limit = nodeNum;
   var skip = 1;
   var returnnum = nodeNum - skip;

   //snapshot with hint/limit/skip
   var actInfo = snapshot( type, filter, selector, hint, limit, skip, returnnum );
   var expInfo = { "num": returnnum, "GroupName": groupName };
   checkResult( expInfo, actInfo );
}

function snapshot ( type, filter, selector, hint, limit, skip, returnnum )
{
   var curlPara = ['cmd=snapshot ' + type,
   "filter=" + filter,
   "selector=" + selector,
   "hint=" + hint,
   "limit=" + limit,
   "skip=" + skip,
   "returnnum=" + returnnum
   ];
   var expErrno = 0;
   runCurl( curlPara );
   var resp = infoSplit;
   var actErrno = JSON.parse( resp[0] ).errno;
   if( expErrno != actErrno )
   {
      throw new Error( "get database by snapshot failed,info = " + infoSplit.toString() );
   }
   resp.shift();
   return resp;
}

function checkResult ( expInfo, actInfo )
{
   var exp = expInfo;
   if( exp.num != actInfo.length )
   {
      throw new Error( "expNum is not equal to actNum,expInfo = " + expInfo + ",actInfo = " + actInfo );
   }
   for( var i = 0; i < actInfo.length; i++ )
   {
      var act = JSON.parse( actInfo[i] );
      if( exp.GroupName != act.GroupName )
      {
         throw new Error( "exp.GroupName is not equal to act.GroupName,expInfo = " + expInfo + ",actInfo = " + actInfo );
      }
   }
}

