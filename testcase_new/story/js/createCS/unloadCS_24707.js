/******************************************************************************
 * @Description   : seqDB-24707:unloadCS，执行find操作
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.13
 * @LastEditTime  : 2022.01.26
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_cs_24707";
testConf.clName = COMMCSNAME + "_cl_24707";

//因为问题单SEQUOIADBMAINSTREAM-7899修改，屏蔽了loadCS和unloadCS接口，该用例暂时屏蔽
//main( test );
function test ( args )
{
   var varCL = args.testCL
   var doc = [];
   doc.push( { a: 1 } );
   varCL.insert( doc );
   db.unloadCS( testConf.csName );

   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      varCL.find().toArray();
   } );

   db.loadCS( testConf.csName );
   var cursor = varCL.find();
   commCompareResults( cursor, doc );
}