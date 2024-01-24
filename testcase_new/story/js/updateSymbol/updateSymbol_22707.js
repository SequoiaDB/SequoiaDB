/*******************************************************************************
*@Description : [seqDB-22707] Connect the data node directly in cluster Environment,
                Update records by using all update symbol, checkout the result.
                在集群环境下直连数据节点，使用所有的更新符更新数据，检查结果
*@Modifier :    2020-09-03  Zixian Yan
*******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.csName = COMMCSNAME
testConf.clName = COMMCLNAME + "_22707";
testConf.clOpt = {};
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var groupName = testPara.srcGroupName;
   var csName = testConf.csName;
   var clName = testConf.clName;
   var dataNode = db.getRG( groupName ).getMaster();
   var hostName = dataNode.getHostName();
   var serviceName = dataNode.getServiceName();

   var tmpSdb = new Sdb( hostName, serviceName );
   var tmpCL = tmpSdb.getCS( csName ).getCL( clName );
   /* Processing Order: $inc $set $unset $addtoset $pop $pull $pull_all $pull_by $pull_all_by $push $push_all $replace
      rawData -> update condition -> expectation */
   var rawData = [{ "_id": 0, "age": 18 }, { "_id": 1, "age": 10 }, { "_id": 2, "data": [1, 2, 3, 4] },
   { "_id": 3, "data": [1, 2, 3, 4] }, { "_id": 4, "data": [1, 2, 3, 4] },
   { "_id": 5, "data": [1, 2, 3, 4], "name": ["Tom", "Mike"] },
   { "_id": 6, "data": [1, 2, 3, 4], "name": ["Tom", "Mike"] },
   { "_id": 7, "data": [1, 2, 3, 2], "datum": [4, 5, 6] }, { "_id": 8, "data": [1, 2, 3, 2], "datum": [4, 5, 6] },
   { "_id": 9, "data": [1, 2, 4], "name": ["Tom", "Mike"] },
   { "_id": 10, "data": [1, 2, 4, 5], "name": ["Tom", "Mike"] },
   { "_id": 11, "data": [5, 5, 6, 6], "name": ["Tom", "Mike"] }];

   var updateOperation = [{ "$inc": { "age": 7 } },
   { "$set": { "age": 8 } },
   { "$unset": { "data.3": "" } },
   { "$addtoset": { "data": [4, 5, 6, 7] } },
   { "$pop": { "data": 2 } },
   { "$pull": { "data": 2, "name": "Tom" } },
   { "$pull_all": { "data": [2, 3], "name": ["Tom"] } },
   { "$pull_by": { "data": 2, "datum": 5 } },
   { "$pull_all_by": { "data": [1, 2], "datum": [5] } },
   { "$push": { "data": 1 } },
   { "$push_all": { "data": [1, 2, 8, 9] } },
   { "$replace": { "DATA": "hasGone" } }];

   var expectation = [{ "age": 25 }, { "age": 8 }, { "data": [1, 2, 3, null] },
   { "data": [1, 2, 3, 4, 5, 6, 7] }, { "data": [1, 2] },
   { "data": [1, 3, 4], "name": ["Mike"] },
   { "data": [1, 4], "name": ["Mike"] },
   { "data": [1, 3], "datum": [4, 6] },
   { "data": [3], "datum": [4, 6] },
   { "data": [1, 2, 4, 1], "name": ["Tom", "Mike"] },
   { "data": [1, 2, 4, 5, 1, 2, 8, 9], "name": ["Tom", "Mike"] },
   { "DATA": "hasGone" }];

   cl.insert( rawData );
   for( var index = 0; index < updateOperation.length; index++ )
   {
      var idObject = { "_id": index };
      var condition = updateOperation[index];
      //Update each row of data with specified condition
      tmpCL.update( condition, idObject );
   }
   // exclude "_id" element
   var rc = cl.find( {}, { "_id": { "$include": 0 } } );
   checkRec( rc, expectation );

   tmpSdb.close();
}
