/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :  seqDB-15733:指定cond查询记录
*                           seqDB-15734:指定sel查询记录
*@auhor       : CSQ   2018-09-20 
*               liuli 2020-10-14 
******************************************************************************/
testConf.clName = COMMCLNAME + "_15733";

main( test );

function test ( args )
{
   var cl = args.testCL;
   insertRecord( cl );
   testcond15733( cl );
   testsel15734( cl );
}

function testcond15733 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( {
      typeint: 123,
      typelong: { "$numberLong": "9223372036854775807" },
      typefloat: 123.456,
      typedecimal: { $decimal: "123.456" },
      typestr: "value",
      typeoid: { "$oid": "123abcd00ef12358902300ef" },
      typebool: true,
      typedate: { "$date": "2012-01-01" },
      typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
      typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
      typeobj: { "subobj": "value" },
      typearr: ["abc", 0, "def"],
      typenull: null,
      typeminkey: { "$minKey": 1 },
      typemaxkey: { "$maxKey": 1 },
      typearr2: [1, [2]],
      typeobj2: { "boj1": { "obj2": "value2" } },
      typearr3: { "name": [0] }
   } ) );
   var expFindResult = [{
      _id: 1,
      typeint: 123,
      typelong: { "$numberLong": "9223372036854775807" },
      typefloat: 123.456,
      typedecimal: { $decimal: "123.456" },
      typestr: "value",
      typeoid: { "$oid": "123abcd00ef12358902300ef" },
      typebool: true,
      typedate: { "$date": "2012-01-01" },
      typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
      typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
      typeregex: { "$regex": "^csq", "$options": "i" },
      typeobj: { "subobj": "value" },
      typearr: ["abc", 0, "def"],
      typenull: null,
      typeminkey: { "$minKey": 1 },
      typemaxkey: { "$maxKey": 1 },
      typearr2: [1, [2]],
      typeobj2: { "boj1": { "obj2": "value2" } },
      typearr3: { "name": [0] }
   }];
   commCompareResults( cur, expFindResult, false );

   cur = varCL.find( new SdbQueryOption().cond( { "typeregex": { $et: { "$regex": "^csq", "$options": "i" } } } ) );
   commCompareResults( cur, expFindResult, false );

   cur = varCL.find( new SdbQueryOption().cond( { $and: [{ "typeint": { $et: 123 } }, { "typefloat": { $gt: 123.456 } }] } ) );
   size = 0;
   while( cur.next() )
   {
      var ret = cur.current();
      size++;
   }
   assert.equal( size, 0 );
}

function testsel15734 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sel( { "typeint": { "$include": 1 } } ) );
   var expFindResult = [{ typeint: 123 }];
   commCompareResults( cur, expFindResult, false );
   //返回多个字段名
   var cur = varCL.find( new SdbQueryOption().sel( { "typeint": { "$include": 1 }, "typefloat": { $include: 1 } } ) );
   var expFindResult = [{ typeint: 123, typefloat: 123.456 }];
   commCompareResults( cur, expFindResult, false );

   //所有类型的字段
   var cur = varCL.find( new SdbQueryOption().sel( {
      typeint: 1, typelong: 1, typefloat: 1,
      typedecimal: 1, typestr: 1, typeoid: 1, typebool: 1, typedate: 1, typetimestamp: 1,
      typebinary: 1, typeregex: 1, typeobj: 1, typearr: 1, typenull: 1, typeminkey: 1,
      typemaxkey: 1, typearr2: 1, typeobj2: 1, typearr3: 1
   } ) );
   var expFindResult = [{
      typeint: 123,
      typelong: { "$numberLong": "9223372036854775807" },
      typefloat: 123.456,
      typedecimal: { $decimal: "123.456" },
      typestr: "value",
      typeoid: { "$oid": "123abcd00ef12358902300ef" },
      typebool: true,
      typedate: { "$date": "2012-01-01" },
      typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
      typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
      typeregex: { "$regex": "^csq", "$options": "i" },
      typeobj: { "subobj": "value" },
      typearr: ["abc", 0, "def"],
      typenull: null,
      typeminkey: { "$minKey": 1 },
      typemaxkey: { "$maxKey": 1 },
      typearr2: [1, [2]],
      typeobj2: { "boj1": { "obj2": "value2" } },
      typearr3: { "name": [0] }
   }];
   commCompareResults( cur, expFindResult, false );

   //指定字段名不存在
   var cur = varCL.find( new SdbQueryOption().sel( { "csq": { "$include": 1 } } ) );
   var expFindResult = [{}];
   commCompareResults( cur, expFindResult, false );
}


function insertRecord ( varCL )
{
   varCL.insert( {
      _id: 1,
      typeint: 123,
      typelong: { "$numberLong": "9223372036854775807" },
      typefloat: 123.456,
      typedecimal: { $decimal: "123.456" },
      typestr: "value",
      typeoid: { "$oid": "123abcd00ef12358902300ef" },
      typebool: true,
      typedate: { "$date": "2012-01-01" },
      typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
      typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
      typeregex: { "$regex": "^csq", "$options": "i" },
      typeobj: { "subobj": "value" },
      typearr: ["abc", 0, "def"],
      typenull: null,
      typeminkey: { "$minKey": 1 },
      typemaxkey: { "$maxKey": 1 },
      typearr2: [1, [2]],
      typeobj2: { "boj1": { "obj2": "value2" } },
      typearr3: { "name": [0] }
   } );
}
