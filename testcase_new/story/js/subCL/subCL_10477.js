/**************************************
 * @Description: 使用upsert带条件更新主子表中不存在的记录
 * @author: ouyangzhongnan 
 * @coverTestcace: 
 *       seqDB-10477:使用upsert带条件更新主子表中不存在的记录
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_upsertByCond_10477.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

main( test );
function test ()
{
   var mainCl = null;
   var subCls = [];
   var data = [];
   var incomeSumByBJ = 0;
   var incomeSumBySH = 0;
   var incomeSumByGZ = 0;
   var mainClName = CHANGEDPREFIX + "maincl_10477";
   var subClNames = [
      CHANGEDPREFIX + "subcl_10477_0",
      CHANGEDPREFIX + "subcl_10477_1"
   ];
   //check test environment before split
   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   //drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );

   //create maincl for range split
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCLOption = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   mainCl = commCreateCL( db, COMMCSNAME, mainClName, mainCLOption, true, true );
   //create subcl
   var subCLOptions = [
      { ReplSize: 0, Compressed: true },
      { ReplSize: 0, Compressed: true, ShardingKey: { a: 1, b: 1 }, ShardingType: "hash", Partition: 16 }
   ];
   //create subcl
   subCls.push( commCreateCL( db, COMMCSNAME, subClNames[0], subCLOptions[0], true, true ) );
   subCls.push( commCreateCL( db, COMMCSNAME, subClNames[1], subCLOptions[1], true, true ) );
   //attach subcl
   attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   //init data
   for( var i = 0; i < 200; i++ )
   {
      data.push( { name: "name" + i, a: i, b: i } );
   }
   mainCl.insert( data );
   ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );

   //upsert test
   var upsertNotExistsData_a50 = { name: "upsertName_a50", a: 50, b: 500 };    //普通表
   var upsertNotExistsData_a150 = { name: "upsertName_a150", a: 150, b: 500 }; //分区表
   mainCl.upsert( { $set: upsertNotExistsData_a50 }, upsertNotExistsData_a50 );
   mainCl.upsert( { $set: upsertNotExistsData_a150 }, upsertNotExistsData_a150 );
   var obj_upsert_a50 = mainCl.find( upsertNotExistsData_a50 ).next().toObj();
   var obj_upsert_a150 = mainCl.find( upsertNotExistsData_a150 ).next().toObj();
   delete obj_upsert_a50["_id"];
   delete obj_upsert_a150["_id"];

   var actName1 = obj_upsert_a50["name"];
   var actA1 = obj_upsert_a50["a"];
   var actB1 = obj_upsert_a50["b"];

   var actName2 = obj_upsert_a150["name"];
   var actA2 = obj_upsert_a150["a"];
   var actB2 = obj_upsert_a150["b"];

   if( actName1 !== "upsertName_a50" || actA1 !== 50 || actB1 !== 500
      || actName2 !== "upsertName_a150" || actA2 !== 150 || actB2 !== 500 )
   {
      throw new Error( "failed to upsert" );
   }

   //drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );

}