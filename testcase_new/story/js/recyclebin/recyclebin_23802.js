/******************************************************************************
 * @Description   : seqDB-23802:CL未开启压缩/snappy压缩，dropCL后恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.02
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

function test ()
{
   var csName = "cs_23802";
   var clName = "cl_23802";
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 指定不开启压缩功能
   var option = { Compressed: false };
   testCompressed( csName, clName, option );

   // 指定开启 snappy 压缩 
   var option = { CompressionType: "snappy" };
   testCompressed( csName, clName, option );
}

function testCompressed ( csName, clName, option )
{
   // 创建 cl 指定压缩类型并插入数据
   var originName = csName + "." + clName;
   var dbcl = commCreateCL( db, csName, clName, option );
   var docs = [];
   for( var i = 0; i < 5000; i += 50 )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   dbcl.insert( docs );

   // 删除 cl 后，恢复并校验数据
   dbcs.dropCL( clName );
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   var dbcl = db.getCS( csName ).getCL( clName );
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}