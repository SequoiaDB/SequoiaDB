/************************************
*@Description: 切分表修改名后，执行数据操作
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16144
**************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var oldcsName = CHANGEDPREFIX + "_16144B_oldcs";
   var newcsName = CHANGEDPREFIX + "_16144B_newcs";
   var clName = CHANGEDPREFIX + "_16144B_CL";
   var fileName = CHANGEDPREFIX + "_16144Blob";
   var lobNum = 10;

   var groupNames = getGroupName( db, true );
   var sourceGroup = groupNames[0][0];
   var targetGroup = groupNames[1][0];

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var options = { ShardingType: "hash", ShardingKey: { a: 1 }, Group: sourceGroup }
   var cl = commCreateCL( db, oldcsName, clName, options, false, false, "create cl in the begin" );

   //insert 1000 data, split 50 to target group
   insertData( cl, 1000 );
   cl.split( sourceGroup, targetGroup, 50 );

   var lobMD5 = createFile( fileName );

   var lobIdArr = putLobs( cl, fileName, lobNum, 5 );

   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );
   cl = db.getCS( newcsName ).getCL( clName );

   checkLob( cl, lobIdArr, lobMD5 );

   deleteLobs( cl, lobIdArr );

   var lobMD5new = createFile( fileName + "_new" );

   var lobArrnew = putLobs( cl, fileName + "_new", lobNum, 5 );

   checkLob( cl, lobArrnew, lobMD5new );

   commDropCS( db, newcsName, true, "clean cs---" );
   deleteFile( fileName );
   deleteFile( fileName + "_new" );
}

function updateData ( cl )
{
   cl.update( { $set: { no: 10086 } } );
   var recordNum = cl.count( { no: 10086 } );
   assert.equal( recordNum, 2000 );
}

function deleteData ( cl )
{
   cl.remove( { a: { $lt: 500 } } );
   var recordNum = cl.count();
   assert.equal( recordNum, 1000 );
}