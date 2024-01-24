/******************************************************************************
 * @Description   :  seqDB-30087:Rest驱动dropCS指定option
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.15
 * @LastEditTime  : 2023.02.16
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
main( test );
function test ()
{
   var csName = "cs_30087";
   var clName = "cl_30087";
   commDropCS( db, csName );

   db.createCS( csName );
   tryCatch( ["cmd=drop collectionspace", "name=" + csName, 'options={EnsureEmpty:false}'], [0] );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );

   db.createCS( csName );
   tryCatch( ["cmd=drop collectionspace", "name=" + csName, 'options={EnsureEmpty:true}'], [0] );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );

   db.createCS( csName ).createCL( clName );
   tryCatch( ["cmd=drop collectionspace", "name=" + csName, 'options={EnsureEmpty:false}'], [0] );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );

   db.createCS( csName ).createCL( clName );
   tryCatch( ["cmd=drop collectionspace", "name=" + csName, 'options={EnsureEmpty:true}'], [SDB_DMS_CS_NOT_EMPTY] );
   db.getCS( csName );

   commDropCS( db, csName );
}