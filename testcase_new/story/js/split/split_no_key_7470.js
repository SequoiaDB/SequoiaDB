/******************************************************************************
@Description : seqDB-7470:range表范围切分时，指定切分键不是分区键
@Modify list :
               2014-07-04  pusheng Ding  Init
               2016-02-18  wuyan changed（add the testcase describe，and add check split result ）
               2020-01-13 huangxiaoni modify
******************************************************************************/
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var groupNames = commGetDataGroupNames( db );
   var srcGroupName = groupNames[0];
   var dstGroupName = groupNames[1];
   var clName = CHANGEDPREFIX + "_split_7470";

   commDropCL( db, COMMCSNAME, clName );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );

   var docs = [
      { "no": 1, "a": { "$minKey": 1 } },  //cminkey = 1
      { "no": 2, "a": undefined },         // cundefined = 2
      { "no": 3, "a": null },              // cnull = 3
      { "no": 4, "a": 0.1 },
      { "no": 5, "a": "test" },
      { "no": 6, "a": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
      { "no": 7, "a": { "$oid": "123abcd00ef12358902300ef" } },
      { "no": 8, "a": false },
      { "no": 9, "a": true },
      { "no": 10, "a": { "$date": "2020-01-01" } },
      { "no": 11, "a": { "$maxKey": 1 } },
      { "no": 12, "a": 1 },
      { "no": 13, "a": 2147483648 },
      { "no": 14, "a": 1.7E+308 },
      { "no": 15, "a": { "$regex": "^张", "$options": "i" } },
      { "no": 16, "a": [1] },
      { "no": 17, "a": { "obj": { "$minKey": 1 } } },
      { "no": 18, "a": { "$timestamp": "2012-01-01-13.14.26.124233" } },
      { "no": 19, "a": { $decimal: "123.456" } },
      { "no": 20 }
   ];
   cl.insert( docs );

   // 切分后分区范围如下，因为范围是undefined，切分数据时先比较数据类型：
   //..."LowBound":{"a":{"$minKey":1}},"UpBound":{"":{"$undefined":1}}
   //..."LowBound":{"":{"$undefined":1}},"UpBound":{"a":{"$maxKey":1}}
   cl.split( srcGroupName, dstGroupName, { "no": 1 } );

   // 校验结果
   var expDocs = [];
   expDocs = expDocs.concat( docs.slice( 0, 1 ) ).concat( [{ "no": 2 }] ).concat( docs.slice( 2, docs.length ) );

   var sort = { "no": 1 };
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( sort );
   commCompareResults( cursor, expDocs );

   checkResultsInGroup( srcGroupName, COMMCSNAME, clName, expDocs.slice( 0, 1 ), sort );
   checkResultsInGroup( dstGroupName, COMMCSNAME, clName, expDocs.slice( 1, expDocs.length ), sort );

   commDropCL( db, COMMCSNAME, clName, true );
}
