/******************************************************************************
 * @Description   : seqDB-31342:使用setConsistencyStrategy修改ConsistencyStrategy参数校验
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.28
 * @LastEditTime  : 2023.05.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_31342";
main( test );
function test ( testPara )
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_31342";

   var dbcl = testPara.testCL;
   //无效值检验
   var invalidValue = 0;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setConsistencyStrategy( invalidValue );
   } );

   invalidValue = 4;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setConsistencyStrategy( invalidValue );
   } );

   invalidValue = "1";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setConsistencyStrategy( invalidValue );
   } );

   invalidValue = "";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setConsistencyStrategy( invalidValue );
   } );

   invalidValue = false;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setConsistencyStrategy( invalidValue );
   } );

   //有效值检验
   var validValues = [1, 2, 3];
   for( var i in validValues )
   {
      dbcl.setConsistencyStrategy( validValues[i] );
      var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
      while( cursor.next() )
      {
         obj = cursor.current().toObj();
         assert.equal( obj.ConsistencyStrategy, validValues[i], JSON.stringify( obj ) );
      }
      cursor.close();
   }
}