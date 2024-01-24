/******************************************************************************
 * @Description   : seqDB-31341:createCL中ConsistencyStrategy参数校验
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.27
 * @LastEditTime  : 2023.05.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ( testPara )
{
   var csName = COMMCSNAME;
   var baseCLName = "cl_31340_";
   var validValues = [1, 2, 3];

   var dbcs = testPara.testCS;
   //无效值检验
   var invalidValue = 0;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcs.createCL( baseCLName, { ConsistencyStrategy: invalidValue } );
   } );

   invalidValue = 4;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcs.createCL( baseCLName, { ConsistencyStrategy: invalidValue } );
   } );

   invalidValue = "1";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcs.createCL( baseCLName, { ConsistencyStrategy: invalidValue } );
   } );

   invalidValue = "";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcs.createCL( baseCLName, { ConsistencyStrategy: invalidValue } );
   } );

   invalidValue = false;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcs.createCL( baseCLName, { ConsistencyStrategy: invalidValue } );
   } );

   //有效值检验
   for( var i in validValues )
   {
      dbcs.createCL( baseCLName + validValues[i], { ConsistencyStrategy: validValues[i] } );
      var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + baseCLName + validValues[i] } );
      while( cursor.next() )
      {
         obj = cursor.current().toObj();
         assert.equal( obj.ConsistencyStrategy, validValues[i], JSON.stringify( obj ) );
      }
      cursor.close();
   }
}