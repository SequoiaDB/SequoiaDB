/***************************************************************************
@Description :seqDB-12071 :通过主表创建了全文索引，删除主表 
@Modify list :
              2018-11-27  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainclName = COMMCLNAME + "_ES_12071";
   dropCL( db, COMMCSNAME, mainclName, true, true );

   //主表及子表在相同和不同的集合空间上
   var mainCL = commCreateCL( db, COMMCSNAME, mainclName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCLName1 = COMMCLNAME + "sub1_cl_12071";
   dropCL( db, COMMCSNAME, subCLName1, true, true );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1 );
   var csName = COMMCSNAME + "sub2_cs_12071";
   dropCS( db, csName );
   var subCLName2 = COMMCLNAME + "sub2_cl_12071";
   var subCL2 = commCreateCL( db, csName, subCLName2 );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 4567 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 4567 }, UpBound: { a: 10001 } } );

   //create index
   commCreateIndex( mainCL, "fullIndex_12071", { b: "text" } );

   //insert records
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: i, b: "b" + i };
      records.push( record );
   }
   mainCL.insert( records );
   checkMainCLFullSyncToES( COMMCSNAME, mainclName, "fullIndex_12071", 10000 )

   //删除主表
   var dbOperator = new DBOperator();
   var esIndexNames1 = dbOperator.getESIndexNames( COMMCSNAME, subCLName1, "fullIndex_12071" );
   var esIndexNames2 = dbOperator.getESIndexNames( csName, subCLName2, "fullIndex_12071" );
   dropCL( db, COMMCSNAME, mainclName, false, false );
   checkIndexNotExistInES( esIndexNames1 );
   checkIndexNotExistInES( esIndexNames2 );

   dropCS( db, csName );
}
