/******************************************************************************
 * @Description   : seqDB-31340:修改集合属性中ConsistencyStrategy参数校验
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.27
 * @LastEditTime  : 2023.05.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_31340";
main( test );
function test ( testPara )
{
   var validValues = [1, 2, 3];

   var dbcl = testPara.testCL;
   //无效值检验
   var invalidValue = 0;
   checkInvalidValue( invalidValue, dbcl );
   invalidValue = 4;
   checkInvalidValue( invalidValue, dbcl );
   invalidValue = "1";
   checkInvalidValue( invalidValue, dbcl );
   invalidValue = "";
   checkInvalidValue( invalidValue, dbcl );
   invalidValue = true;
   checkInvalidValue( invalidValue, dbcl );
   //有效值检验
   checkValidValues( db, validValues, dbcl );
}

function checkValidValues ( db, validValues, dbcl )
{
   var fullName = dbcl.toString();
   for( var i in validValues )
   {
      dbcl.alter( { ConsistencyStrategy: validValues[i] } );
      var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: fullName } );
      while( cursor.next() )
      {
         obj = cursor.current().toObj();
         assert.equal( obj.ConsistencyStrategy, validValues[i], JSON.stringify( obj ) );
      }
      cursor.close();

      dbcl.setAttributes( { ConsistencyStrategy: validValues[i] } );

      var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: fullName } );
      while( cursor.next() )
      {
         obj = cursor.current().toObj();
         assert.equal( obj.ConsistencyStrategy, validValues[i], JSON.stringify( obj ) );
      }
      cursor.close();
   }
}

function checkInvalidValue ( invalidValue, dbcl )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.alter( { ConsistencyStrategy: invalidValue } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setAttributes( { ConsistencyStrategy: invalidValue } );
   } );
}