/*******************************************************************************
*@Description : [jira_330] [seqDB-22587].Query by using sql sentence with like.
                使用 Sql 语句代替 查询条件， 用 like 代替 $regex
*@Modifier :
*               2014-11-10  xiaojun Hu  Init
                2020-08-08  Zixian Yan
*******************************************************************************/
testConf.csName = COMMCSNAME + "_sql_22587";
testConf.clName = COMMCLNAME + "_sql_22587";
testConf.useSrcGroup = true;
testConf.clOpt = {};
main( test );

function test ( testPara )
{
   var indexName = "index_sql_22587";
   var cl = testPara.testCL;
   var groups = testPara.groups;
   var clGroupName = testPara.srcGroupName;

   var data = [];
   for( var i = 0; i < 26; i++ )
   {
      //alphabet: A-Z
      var character = String.fromCharCode( 65 + i );
      data.push( { "regex": "abcdefg" + character + "test" } );
   }

   cl.insert( data );
   cl.createIndex( indexName, { "regex": 1 } );

   // query by using regex , run db.exec( <sql string> ) ;
   var cs_clName = testConf.csName + "." + testConf.clName;
   var sqlCondition = [" select * from " + cs_clName + " where regex like '^abcdefg*h' ",
   " select * from " + cs_clName + " where regex like '^abcdefg*Htest' ",
   " select * from " + cs_clName + " where regex like '\\AabcdefgZ' "];
   var querylenList = [0, 1, 1];

   var idxRead1 = queryGetCurrentSessions( db, cs_clName, clGroupName, groups );
   for( var i in sqlCondition )
   {
      var sql = sqlCondition[i];
      var queryLen = querylenList[i];
      var queryNum = db.exec( sql ).toArray();
      var idxRead2 = queryGetCurrentSessions( db, cs_clName, clGroupName, groups );

      //Error judgement
      if( queryLen != queryNum.length )
      {
         throw new Error( "ErrQueryNum" );
      }
      if( idxRead2[1] <= idxRead1[1] && idxRead2[0] == idxRead1[0] )
      {
         throw new Error( "Can't query from index, Error" );
      }

      idxRead1 = idxRead2;
   }

}

function queryGetCurrentSessions ( db, cs_clName, clGN, groups )
{
   var masNode = new Array;
   var masHost = new Array;
   for( var j = 0; j < groups.length; ++j )
   {
      if( groups[j][0].GroupName == clGN )
      {
         for( var m = 0; m < groups[j][0].Length; ++m )
         {
            masNode[m] = groups[j][m + 1].svcname;
            masHost[m] = groups[j][m + 1].HostName;
         }
      }
   }
   var currentSession = db.snapshot( SDB_SNAP_SESSIONS_CURRENT, {},
      {
         "NodeName": 1, "SessionID": 1, "TotalIndexRead": 1,
         "TotalDataRead": 1
      } ).toArray();
   var dataIdx = new Array( 0, 0 );
   // get total data read and total index read in this loop
   for( var i = 0; i < currentSession.length; ++i )
   {
      var sessionObj = eval( "(" + currentSession[i] + ")" );
      var _nodeName = sessionObj["NodeName"];
      // get host and split
      var nodeSplit = _nodeName.split( ":" );
      // cluster environment
      if( false == commIsStandalone( db ) )
      {
         for( var j = 0; j < masNode.length; ++j )
         {
            if( masNode[j] == nodeSplit[1] && masHost[j] == nodeSplit[0] )
            {
               dataIdx[0] += sessionObj.TotalDataRead;
               dataIdx[1] += sessionObj.TotalIndexRead;
               break;
            }
         }
      }
      // standalone
      else
      {
         dataIdx[0] = sessionObj.TotalDataRead;
         dataIdx[1] = sessionObj.TotalIndexRead;
      }
   }

   return dataIdx;
}
