/************************************
*@Description: detach子表后，验证数据操作
*@author:      wangkexin
*@createDate:  2019.3.20
*@testlinkCase: seqDB-66
**************************************/
main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var mainCL_Name = "maincl66";
   var subCL_Name = "subcl66";
   var subCL_num = 3;
   var coordGroup = commGetGroups( db, true, "SYSCoord", true, false, true );
   var nodeNum = eval( coordGroup[0].length - 1 );
   if( nodeNum < 2 )
   {
      return;
   }

   var hostname1 = coordGroup[0][1].HostName;
   var svcname1 = coordGroup[0][1].svcname;

   var hostname2 = coordGroup[0][2].HostName;
   var svcname2 = coordGroup[0][2].svcname;

   var db1 = new Sdb( hostname1, svcname1 );
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var maincl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );
   createAndAttachSubCL( db1, COMMCSNAME, subCL_Name, maincl, subCL_num );

   insertData( maincl );
   maincl.detachCL( COMMCSNAME + "." + subCL_Name + "_0" );

   //连接另一个coord节点通过主表增删改查分离出的子表中的数据
   var db2 = new Sdb( hostname2, svcname2 );
   var maincl2 = db2.getCS( COMMCSNAME ).getCL( mainCL_Name );
   var detachedcl = db2.getCS( COMMCSNAME ).getCL( subCL_Name + "_0" );
   crudAndCheckDetached( maincl2, detachedcl );

   //连接另一个coord节点通过主表增删改查其他子表中的数据
   var subcl2 = db2.getCS( COMMCSNAME ).getCL( subCL_Name + "_1" );
   crudAndCheck( maincl2, subcl2 );

   //清除环境
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "drop CL in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name + "_0", true, true, "drop detachCL in the end" );
}

function createAndAttachSubCL ( db, csName, subclName, mainCL, clNum )
{
   var lowBound = 0;
   for( var i = 0; i < clNum; i++ )
   {
      var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 };
      commCreateCL( db, csName, subclName + "_" + i, subClOption, true, true );

      var options = { LowBound: { a: lowBound }, UpBound: { a: parseInt( lowBound ) + 10 } };
      mainCL.attachCL( csName + "." + subclName + "_" + i, options );
      lowBound += 10;
   }
}

function insertData ( cl )
{
   var data = [];
   for( var i = 0; i < 30; i++ )
   {
      data.push( { a: i } );
   }
   cl.insert( data );
}

function crudAndCheckDetached ( maincl, subcl )
{
   var obj = { a: 5 };
   //CRUD operation
   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      maincl.insert( obj );
   } )

   maincl.remove( obj );
   var cursor = subcl.findOne( obj );
   assert.notEqual( cursor.next(), undefined );

   subcl.insert( { field: 123 } );
   maincl.update( { $set: { field: 111 } }, { field: 123 } );
   cursor = subcl.findOne( { field: 111 } );
   assert.equal( cursor.next(), undefined );

   cursor = maincl.find( obj );
   assert.equal( cursor.next(), undefined );
   cursor.close();
}

function crudAndCheck ( maincl, subcl )
{
   //CRUD operation
   //query
   var cursor = subcl.find().sort( { a: 1 } );
   var i = 10;
   while( cursor.next() != null )
   {
      var expObj = { a: i };
      assert.equal( cursor.current().toObj()["a"], expObj["a"] );
      i++;
   }

   //remove
   maincl.remove();
   cursor = subcl.find();
   assert.equal( cursor.next(), undefined );

   //insert
   var obj = { a: 15 };
   maincl.insert( obj );
   assert.equal( maincl.count(), 1 );
   cursor = subcl.find( obj );

   assert.equal( cursor.next().toObj()["a"], obj["a"] );

   //update
   subcl.insert( { testField: 1 } );
   maincl.update( { $set: { testField: 2 } }, { testField: 1 } );
   obj = { testField: 2 };
   cursor = maincl.find( obj );
   assert.equal( cursor.next().toObj()["testField"], obj["testField"] );

   cursor.close();
}