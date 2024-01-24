/************************************
*@Description: 索引名取边界值1023字节，验证基本操作
*@author:      wangkexin
*@createdate:  2019.5.28
*@testlinkCase:seqDB-18357
**************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_indexcl18357";
   commDropCL( db, csName, clName, true, true, "clear cl in the beginning" );

   var cl = commCreateCL( db, csName, clName );
   var single_index128 = "";
   var composite_index128 = "";
   var single_index1023 = "";
   var composite_index1023 = "";
   for( var i = 0; i < 1023; i++ )
   {
      single_index128 += 'c';
      composite_index128 += 'd';
   }

   for( var i = 0; i < 1023; i++ )
   {
      single_index1023 += 'e';
      composite_index1023 += 'f';
   }

   //索引长度为1，索引长度为128，索引长度为1023
   testCreateIndex( cl, "a", "b" );
   testCreateIndex( cl, single_index128, composite_index128 );
   testCreateIndex( cl, single_index1023, composite_index1023 );

   commDropCL( db, csName, clName, true, true, "clear cl in the end" )
}

function testCreateIndex ( cl, single_index, composite_index )
{
   //-------test single index
   cl.createIndex( single_index, { 'a': 1 }, true );

   //crud operation
   var insertNum = 5000;
   var expRemainNum = insertNum - 1;
   var obj = { a: 1 };
   var newObj = { a: 123456789 };
   testInsert( cl, insertNum, single_index );
   var curObj = testUpdate( cl, obj, newObj );
   testDelete( cl, curObj, expRemainNum );

   //-------test composite index
   cl.remove();
   cl.createIndex( composite_index, { 'b': 1, 'c': -1 }, true );

   //crud operation
   var obj2 = { b: 1, c: "test1" };
   var newObj2 = { b: 123456789, c: "newtest18357" };
   testInsert2( cl, insertNum, composite_index );
   var curObj2 = testUpdate( cl, obj2, newObj2 );
   testDelete( cl, curObj2, expRemainNum );

   cl.remove();
   cl.dropIndex( single_index );
   cl.dropIndex( composite_index );
}

function testInsert ( cl, insertNum, idxName )
{
   var expRecs = [];
   for( var i = 0; i < insertNum; i++ )
   {
      var obj = { a: i };
      expRecs.push( obj );
   }
   cl.insert( expRecs );
   var rc = cl.find().sort( { a: 1 } ).hint( { "": idxName } );
   checkRec( rc, expRecs );
}

function testInsert2 ( cl, insertNum, idxName )
{
   var expRecs = [];
   for( var i = 0; i < insertNum; i++ )
   {
      var obj = { b: i, c: "test" + i };
      expRecs.push( obj );
   }
   cl.insert( expRecs );
   var rc = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { b: 1 } ).hint( { "": idxName } );
   checkRec( rc, expRecs );
}

function testUpdate ( cl, oldObj, newObj )
{
   var expRecs = [];
   expRecs.push( newObj );
   cl.update( { $set: newObj }, oldObj );
   var count1 = cl.find( oldObj ).count();
   assert.equal( count1, 0 );

   var count2 = cl.find( newObj ).count();
   assert.equal( count2, 1 );

   return newObj;
}

function testDelete ( cl, keyValue, expRemainNum )
{
   cl.remove( keyValue );
   var count = cl.find( keyValue ).count();
   assert.equal( count, 0 );

   var actRemainNum = cl.count();
   assert.equal( expRemainNum, actRemainNum );

}