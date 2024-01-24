/******************************************************************************
 * @Description   : seqDB-16714:options取值验证
 * @Author        : Wang Kexin
 * @CreateTime    : 2018.11.26
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_16714";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var oid = "5bf7575bdc4e88fa3dd16714";
   var obj1 = { _id: 1, a: 1, b: 1 };
   // 预期插入返回值
   var obj2 = { "InsertedNum": 1, "DuplicatedNum": 0 };
   // 默认返回值对比
   var returnString = varCL.insert( obj1 );
   var actual = returnString.toObj().toString();
   assert.equal( actual, obj2 );

   // ReturnOID is true
   var obj3 = { "_id": oid, "test": "test16714" };
   var returnOidString = varCL.insert( obj3, { ReturnOID: true } );
   var actua2 = returnOidString.toObj()._id.toString();
   assert.equal( actua2, oid );

   // ReturnOID is false
   var obj4 = { "name": "Tom", "age": "20" };
   var returnOidNull = varCL.insert( obj4, { ReturnOID: false } );
   var actua3 = returnOidNull.toObj().toString();
   assert.equal( actua3, obj2 );

   //  ContOnDup is true
   varCL.insert( [{ "_id": oid, test: "test16714_1" }, obj4], { ContOnDup: true } );
   var cursor = varCL.find( { "_id": oid } );
   commCompareResults( cursor, [{ "_id": oid, test: "test16714" }], false );

   // ContOnDup is false
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      varCL.insert( obj1, { ContOnDup: false } );
   } )
}