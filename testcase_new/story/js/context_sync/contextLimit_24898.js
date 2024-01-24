/******************************************************************************
 * @Description   : seqDB-24898:节点shard平面单节点开启上下文超过限制 
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.01.07
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var mainClName = COMMCLNAME + "mainCl_24893";
   var subClName = COMMCLNAME + "cl_24893_";
   var clNum = 200;
   var groupName = commGetDataGroupNames( db )[0];

   for( var i = 0; i < clNum; i++ )
   {
      commDropCL( db, COMMCSNAME, subClName + i, true, true );
   }
   commDropCL( db, COMMCSNAME, mainClName, true, true );

   var mainClOptionObj = {
      "ShardingKey": { "no": 1 }, "ShardingType": "range", "IsMainCL": true, "Group": groupName
   };
   var option = { "Group": groupName }
   var mainCL = commCreateCL( db, COMMCSNAME, mainClName, mainClOptionObj, true, false );
   for( var i = 0; i < clNum; i++ )
   {
      commCreateCL( db, COMMCSNAME, subClName + i, option, true, false );
      mainCL.attachCL( COMMCSNAME + "." + subClName + i, {
         LowBound: { no: i },
         UpBound: { no: i + 1 }
      } );
   }

   insertData( mainCL, clNum );
   var maxSessionContextNum = 100;
   var config = { "maxsessioncontextnum": maxSessionContextNum };
   db.updateConf( config );

   try
   {
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME )
      var mainCl1 = db1.getCS( COMMCSNAME ).getCL( mainClName );
      // 非分区键排序，所有200个子表context一起打开
      mainCl1.find().sort( { a: 1 } ).explain( { Run: true, Detail: true } )
   }
   finally
   {
      db1.close();
      db.deleteConf( { "maxsessioncontextnum": 1 } );
   }

   for( var i = 0; i < clNum; i++ )
   {
      commDropCL( db, COMMCSNAME, subClName + i, false, false );
   }
   commDropCL( db, COMMCSNAME, mainClName, false, false );
}