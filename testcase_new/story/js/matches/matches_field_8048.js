/************************************************************************
*@Description:   seqDB-8048:使用$field查询，t1字段和t2字段为不同数据类型
                    cover all data type
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8048";
   var cl = readyCL( clName );

   var rawData = insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   var rawData = [{
      a: 0,
      int: 2147483647,
      double: 2147483647.00,
      long: { "$numberLong": "2147483647" },
      decimal: { "$decimal": "2147483647.00" }
   }];
   cl.insert( rawData );

   return rawData;
}

function findRecs ( cl )
{
   dataType = ["int", "double", "long", "decimal"];
   var rmNum1 = parseInt( Math.random() * dataType.length );
   var rmNum2 = parseInt( Math.random() * dataType.length );

   //field variable
   var cond = new Object();
   var field1 = dataType[rmNum1];
   cond[field1] = { $field: dataType[rmNum2] };
   //condition of find
   //find
   var rc = cl.find( cond, { _id: { $include: 0 } } ).sort( { a: 1 } );

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   var expLen = 1;
   assert.equal( findRecsArray.length, expLen );
   var actRecs = JSON.stringify( findRecsArray[0] );
   var extRecs = '{"a":0,"int":2147483647,"double":2147483647,"long":2147483647,"decimal":{"$decimal":"2147483647.00"}}';
   assert.equal( actRecs, extRecs );
}