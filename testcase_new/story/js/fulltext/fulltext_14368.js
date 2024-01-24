/***************************************************************************
@Description :seqDB-14368 :创建全文索引，固定集合名验证 
@Modify list :
              2018-10-25  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_14368";
   var csName = "testCS_ES_14368";
   dropCL( db, COMMCSNAME, clName, true, true );
   dropCS( db, csName, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var dbcl1 = commCreateCL( db, csName, clName );
   var dbcl2 = commCreateCL( db, csName, clName + "_2", { ShardingKey: { content: 1 }, ShardingType: "hash" } )
   var dbcl3 = commCreateCL( db, csName, clName + "_3", { ShardingKey: { content: 1 }, ShardingType: "range" } )
   var subCL1 = commCreateCL( db, csName, clName + "_4_1" );
   var subCL2 = commCreateCL( db, csName, clName + "_4_2" );
   var mainCL = commCreateCL( db, csName, clName + "_4", { ShardingKey: { content: 1 }, ShardingType: "range", IsMainCL: true } )
   mainCL.attachCL( csName + "." + clName + "_4_1", { LowBound: { content: "a" }, UpBound: { content: "f" } } );
   mainCL.attachCL( csName + "." + clName + "_4_2", { LowBound: { content: "x" }, UpBound: { content: "z" } } );

   //在不同的集合空间，不同的集合创建全文索引
   var indexName = "a_14368";
   commCreateIndex( dbcl, indexName, { content: "text" } );
   commCreateIndex( dbcl1, indexName, { content: "text" } );
   commCreateIndex( dbcl2, indexName, { content: "text" } );
   commCreateIndex( dbcl3, indexName, { content: "text" } );
   commCreateIndex( mainCL, indexName, { content: "text" } );

   //获取固定集合名
   var dbOperator = new DBOperator();
   var cappedArray = new Array();
   var cappedCLName = dbOperator.getCappedCLName( dbcl, indexName );
   cappedArray.push( cappedCLName );
   var cappedCLName1 = dbOperator.getCappedCLName( dbcl1, indexName );
   cappedArray.push( cappedCLName1 );
   var cappedCLName2 = dbOperator.getCappedCLName( dbcl2, indexName );
   cappedArray.push( cappedCLName2 );
   var cappedCLName3 = dbOperator.getCappedCLName( dbcl3, indexName );
   cappedArray.push( cappedCLName3 );
   var cappedSubCLName1 = dbOperator.getCappedCLName( subCL1, indexName );
   cappedArray.push( cappedSubCLName1 );
   var cappedSubCLName2 = dbOperator.getCappedCLName( subCL2, indexName );
   cappedArray.push( cappedSubCLName2 );

   //检查固定集合名均不一致
   for( var i in cappedArray )
   {
      var cpName = cappedArray[i];
      if( cappedArray.indexOf( cpName ) != cappedArray.lastIndexOf( cpName ) )
      {
         throw new Error( "exists duplicate capped cl name, cappedName: " + cpName + "in cappedArray: " + JSON.stringify( cappedArray ) );
      }
   }

   //检查索引属性的ExtDataName和固定集合名一致
   checkExtDataName( dbcl, indexName, cappedCLName );
   checkExtDataName( dbcl1, indexName, cappedCLName1 );
   checkExtDataName( dbcl2, indexName, cappedCLName2 );
   checkExtDataName( dbcl3, indexName, cappedCLName3 );
   checkExtDataName( subCL1, indexName, cappedSubCLName1 );
   checkExtDataName( subCL2, indexName, cappedSubCLName2 );

   //检查ES端的全文索引名字映射关系为固定集合名_组名
   var esOperator = new ESOperator();
   var groups = commGetCLGroups( db, COMMCSNAME + "." + clName );
   var esIndexName1 = FULLTEXTPREFIX.toLowerCase() + cappedCLName.toLowerCase() + "_" + groups[0];
   esOperator.isCreateIndexInES( esIndexName1 );
   var groups = commGetCLGroups( db, csName + "." + clName );
   var esIndexName2 = FULLTEXTPREFIX.toLowerCase() + cappedCLName1.toLowerCase() + "_" + groups[0];
   esOperator.isCreateIndexInES( esIndexName2 );
   var groups = commGetCLGroups( db, csName + "." + clName + "_2" );
   var esIndexName3 = FULLTEXTPREFIX.toLowerCase() + cappedCLName2.toLowerCase() + "_" + groups[0];
   esOperator.isCreateIndexInES( esIndexName3 );
   var groups = commGetCLGroups( db, csName + "." + clName + "_3" );
   var esIndexName4 = FULLTEXTPREFIX.toLowerCase() + cappedCLName3.toLowerCase() + "_" + groups[0];
   esOperator.isCreateIndexInES( esIndexName4 );
   var groups = commGetCLGroups( db, csName + "." + clName + "_4_1" );
   var esIndexName5 = FULLTEXTPREFIX.toLowerCase() + cappedSubCLName1.toLowerCase() + "_" + groups[0];
   esOperator.isCreateIndexInES( esIndexName5 );
   var groups = commGetCLGroups( db, csName + "." + clName + "_4_2" );
   var esIndexName6 = FULLTEXTPREFIX.toLowerCase() + cappedSubCLName2.toLowerCase() + "_" + groups[0];
   esOperator.isCreateIndexInES( esIndexName6 );

   dropCL( db, COMMCSNAME, clName, true, true );
   dropCS( db, csName, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexName1 );
   checkIndexNotExistInES( esIndexName2 );
   checkIndexNotExistInES( esIndexName3 );
   checkIndexNotExistInES( esIndexName4 );
   checkIndexNotExistInES( esIndexName5 );
   checkIndexNotExistInES( esIndexName6 );
}

function checkExtDataName ( dbcl, indexName, cappedCLName )
{
   var index = dbcl.getIndex( indexName ).toObj();
   var extDataName = index["ExtDataName"];
   if( cappedCLName != extDataName )
   {
      throw new Error( "index's property ExtDataName is not equal to cappedCLName, cappedName: " + cappedCLName + ",extDataName: " + extDataName );
   }
}
