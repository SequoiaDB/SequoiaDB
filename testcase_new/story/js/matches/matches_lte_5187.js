/*******************************************************************************
*@Description: find操作，匹配符$lte，二进制/正则表达式/OID/对象类型校验_ST.basicOperate.find.lte.005
*@Author:  2019-6-4  wangkexin
*@testlinkCase: seqDB-5187
********************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5187";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var cl = commCreateCL( db, csName, clName );
   var expRecs1 = insertData( cl, "binary" );
   var matchValue1 = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "50" };
   var rc1 = cl.find( { key: { "$lte": matchValue1 } } );
   checkRec( rc1, expRecs1 );

   var expRecs2 = insertData( cl, "regex" );
   var matchValue2 = { "$regex": "^W", "$options": "m" };
   var rc2 = cl.find( { key: { "$lte": matchValue2 } } );
   checkRec( rc2, expRecs2 );

   var expRecs3 = insertData( cl, "oid" );
   var matchValue3 = { "$oid": "5156c192f970aed30c020010" };
   var rc3 = cl.find( { key: { "$lte": matchValue3 } } );
   checkRec( rc3, expRecs3 );

   var expRecs4 = insertData( cl, "object" );
   var matchValue4 = { "a": 2 };
   var rc4 = cl.find( { key: { "$lte": matchValue4 } } );
   checkRec( rc4, expRecs4 );

   //drop cl
   commDropCL( db, csName, clName, true, true, "drop cl in the end" );
}

function insertData ( cl, type )
{
   cl.remove();
   var expRecs = [];
   var dataArray = new Array();
   switch( type )
   {
      case "binary":
         //以下二进制数据原始数据（编码前）为"test5187"、"hello world"和"binary data"
         dataArray.push( { "_id": 0, "key": { "$binary": "dGVzdDUxODc=", "$type": "0" } } );
         dataArray.push( { "_id": 1, "key": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "50" } } );
         dataArray.push( { "_id": 2, "key": { "$binary": "YmluYXJ5IGRhdGE=", "$type": "255" } } );
         break;
      case "regex":
         dataArray.push( { "_id": 0, "key": { "$regex": "^W", "$options": "i" } } );
         dataArray.push( { "_id": 1, "key": { "$regex": "^W", "$options": "m" } } );
         dataArray.push( { "_id": 2, "key": { "$regex": "^W", "$options": "s" } } );
         break;
      case "oid":
         dataArray.push( { "_id": 0, "key": { "$oid": "5156c192f970aed30c020000" } } );
         dataArray.push( { "_id": 1, "key": { "$oid": "5156c192f970aed30c020010" } } );
         dataArray.push( { "_id": 2, "key": { "$oid": "5156c192f970aed30c020060" } } );
         break;
      case "object":
         dataArray.push( { "_id": 0, "key": { "a": 1 } } );
         dataArray.push( { "_id": 1, "key": { "a": 2 } } );
         dataArray.push( { "_id": 2, "key": { "b": 1 } } );
         break;
      default:
         throw new Error("unexpected type");
         break;
   }
   cl.insert( dataArray );
   //不同数据类型，插入记录数都为3条，当匹配值为中间值时，expRecs应为前两条记录
   expRecs.push( dataArray[0] );
   expRecs.push( dataArray[1] );
   return expRecs;
}
