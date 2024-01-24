/******************************************************************************
 * @Description   : seqDB-23790:恢复的项目在DATA上不存在
 * @Author        : liuli
 * @CreateTime    : 2021.04.22
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23790";
   var clName = "cl_23790";
   var groupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   commCreateCL( db, csName, clName, { Group: groupNames[0] } );

   // 删除CL后删除data上的项目
   dbcs.dropCL( clName );

   // 删除data项目后恢复
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   var data = db.getRG( groupNames[0] ).getMaster().connect();
   data.getCS( csName ).dropCL( recycleName );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   data.close();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}