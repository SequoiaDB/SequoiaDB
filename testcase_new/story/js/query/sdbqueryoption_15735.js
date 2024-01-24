/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :  seqDB-15735:指定sort查询记录
*                           seqDB-15736:指定hint查询记录
*                           seqDB-15737:指定skip查询记录
*                           seqDB-15738:指定limit查询记录
*                           seqDB-15739:指定remove查询记录
*@auhor       : CSQ   2018-09-20 
*               liuli 2020-10-16
******************************************************************************/
testConf.clName = COMMCLNAME + "_15749";

main( test );

function test ( args )
{
   var cl = args.testCL;
   insertRecord( cl );
   testsort15735( cl );
   testhint15736( cl );
   testskip15737( cl );
   testlimit15738( cl );
   testremove15739( cl );
}

function testsort15735 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   commCompareResults( cur, expFindResult1, false );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: -1 } ) );
   var expFindResult2 = [
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      }
   ];
   commCompareResults( cur, expFindResult2, false );

   var cur = varCL.find( new SdbQueryOption().sort( { "typeobj.subobj": -1, "typeobj2.boj1": 1, "typearr3": -1, "typefloat": 1 } ) );
   commCompareResults( cur, expFindResult2, false );
}

function testhint15736 ( varCL )
{
   varCL.createIndex( "typefloatIndex", { typefloat: 1 }, false );
   var cur = varCL.find( new SdbQueryOption().hint( { "": "typefloatIndex" } ).sort( { typeint: 1 } ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   commCompareResults( cur, expFindResult1, false );

}

function testskip15737 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).skip( 0 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   commCompareResults( cur, expFindResult1, false );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).skip( 2 ) );
   var expFindResult1 = [
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   commCompareResults( cur, expFindResult1, false );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).skip( 10 ) );
   var size = 0;
   while( cur.next() )
   {
      var ret = cur.current();
      size++;
   }
   assert.equal( size, 0 );
}

function testlimit15738 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).limit( 1 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      }
   ];
   commCompareResults( cur, expFindResult1, false );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).limit( 2 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      }
   ];
   commCompareResults( cur, expFindResult1, false );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).limit( 10 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   commCompareResults( cur, expFindResult1, false );
}

function testremove15739 ( varCL )
{
   var rc = varCL.find( new SdbQueryOption().cond( { "typeint": { "$gt": 0 } } ).remove() ).toArray();
   var act = varCL.find().count();
   assert.equal( act, 0 );
}

function insertRecord ( varCL )
{
   for( var i = 1; i <= 5; i++ )
   {
      varCL.insert( {
         _id: i,
         typeint: i,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * i,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + i,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + i },
         typearr: ["abc" + i],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [i * -1] }],
         typeobj2: { "boj1": { "obj2": "value2" + i } },
         typearr3: ["name", [i]]
      } );
   }
}

