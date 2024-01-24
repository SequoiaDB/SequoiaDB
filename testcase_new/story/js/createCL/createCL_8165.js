/******************************************************************************
*@Description : createCL basic testcase
*@Modify list :
*               2015-01-22   Pusheng Ding Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   csName_1 = CHANGEDPREFIX + "_normal" + "_8165";
   csName_2 = CHANGEDPREFIX + "_rangecl" + "_8165";
   csName_3 = CHANGEDPREFIX + "_hashcl" + "_8165";
   csName_4 = CHANGEDPREFIX + "_maincl" + "_8165";
   csName_5 = CHANGEDPREFIX + "_subcl1" + "_8165";
   csName_6 = CHANGEDPREFIX + "_subcl2" + "_8165";

   commDropCL( db, COMMCSNAME, csName_1 );
   commDropCL( db, COMMCSNAME, csName_2 );
   commDropCL( db, COMMCSNAME, csName_3 );
   commDropCL( db, COMMCSNAME, csName_5 );
   commDropCL( db, COMMCSNAME, csName_6 );
   // main cl
   commDropCL( db, COMMCSNAME, csName_4 );

   var cs = db.getCS( COMMCSNAME );
   cs.createCL( csName_1, { Compressed: true } );
   cs.createCL( csName_2, { Compressed: false, ShardingType: "range", ShardingKey: { a: 1, b: 1 } } );
   cs.createCL( csName_3, { ShardingType: "hash", ShardingKey: { a: 1 }, Partition: 4096, Compressed: true, AutoSplit: true } );
   var mainCL = cs.createCL( csName_4, { ShardingKey: { id: 1 }, IsMainCL: true } );
   cs.createCL( csName_5 );
   cs.createCL( csName_6, { ShardingKey: { a: 1, b: -1 }, ShardingType: "hash", Compressed: true } );
   mainCL.attachCL( COMMCSNAME + "." + csName_5, { LowBound: { id: { "$minKey": 1 } }, UpBound: { id: 0 } } );
   mainCL.attachCL( COMMCSNAME + "." + csName_6, { LowBound: { id: 0 }, UpBound: { id: { "$maxKey": 1 } } } );

   commDropCL( db, COMMCSNAME, csName_1 );
   commDropCL( db, COMMCSNAME, csName_2 );
   commDropCL( db, COMMCSNAME, csName_3 );
   commDropCL( db, COMMCSNAME, csName_5 );
   commDropCL( db, COMMCSNAME, csName_6 );
   // main cl
   commDropCL( db, COMMCSNAME, csName_4 );
}