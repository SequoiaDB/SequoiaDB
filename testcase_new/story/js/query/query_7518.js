/*******************************************************************************
*@Description : check the times of index read when query, if it hasn't save the
*               context, duplicate index read will happen. [jira_201]
*@Modify list :
*               2014-11-6  xiaojun Hu  Changed
*               2019-08-23 wangkexin Modified
                2020-08-17 Zixian Yan  Modified
*******************************************************************************/
testConf.clName = COMMCLNAME + "_7518";
testConf.useSrcGroup = true;
testConf.clOpt = {};

main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var cs_clName = testConf.csName + "." + testConf.clName;
   var groups = testPara.groups;
   var clGroupName = testPara.srcGroupName;

   // insert data
   var count = 2000;
   var records = [];
   for( var i = 1; i <= count; i++ )
   {
      records.push( { id: i, pid: i, name: "mike" + i + "index" + i, content: "afdsafdsafdsafdsafdsafdsafdsafdsafdsafdsafdsafdsa", uid: i } );
   }
   cl.insert( records );

   // create index
   cl.createIndex( "idIndex", { id: 1 }, false );
   cl.createIndex( "idpidIndex", { id: -1, pid: 1 }, false );
   cl.createIndex( "idpidnameIndex", { id: 1, pid: 1, name: 1 }, true );

   var snapInfo = getCurrentRead( db, cs_clName, clGroupName, groups );
   // "==>query1. {Index Name : 'idIndex'}"
   // "==>query2. {Index Name : 'idpidIndex'}"
   // "==>query3. {Index Name : 'idpidnameIndex'}"
   // "==>query4. {Index Name : null}"
   var findCondition = [{ id: 1000 },
   { id: { $lt: 5000, $gt: 2000 }, pid: 10 },
   { $and: [{ id: 1000 }, { pid: { $lt: 10 } }, { name: "mike1000index3" }] },
   { $and: [{ pid: { $gt: 1 } }, { uid: 50 }] }];

   var comparisonValue = [cl.count() * 5, cl.count() * 2, 20, 0]

   for( var i in findCondition )
   {
      var conditon = findCondition[i];
      var comparison = comparisonValue[i];

      var dataRead1 = snapInfo[0];
      var indexRead1 = snapInfo[1];

      cl.find( conditon ).next();
      var snapInfo2 = getCurrentRead( db, cs_clName, clGroupName, groups );
      var dataRead2 = snapInfo2[0];
      var indexRead2 = snapInfo2[1];

      var dataVal = dataRead2 - dataRead1;
      var indexVal = indexRead2 - indexRead1;

      snapInfo = snapInfo2;

      if( i == 3 )
      {
         if( indexVal > comparison )
         {
            throw new Error( "Error,this operator can't be use the index read" );
         }
      }
      else
      {
         if( indexVal == 0 )
         {
            throw new Error( "Error,this operator can't be use the index read" );
         }
         if( indexVal > comparison )
         {
            throw new Error( "Read index too much times" );
         }
      }
   }
}

function getCurrentRead ( db, cs_clName, clGN, groups )
{
   var masNode = new Array;
   var masHost = new Array;
   for( var j = 0; j < groups.length; ++j )
   {
      if( groups[j][0].GroupName == clGN )
      {
         for( var m = 0; m < groups[j][0].Length; ++m )
         {
            // get Master's HostName & ServiceName
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
      var nodeSplit = _nodeName.split( ":" );
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
