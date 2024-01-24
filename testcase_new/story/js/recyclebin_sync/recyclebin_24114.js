/******************************************************************************
 * @Description   : seqDB-24114:AutoDrop参数校验
 * @Author        : liuli
 * @CreateTime    : 2021.04.20
 * @LastEditTime  : 2022.02.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   try
   {
      alter();
   }
   finally
   {
      db.getRecycleBin().setAttributes( { AutoDrop: false } );
   }
}

function alter ()
{
   var autoDrop = db.getRecycleBin().getDetail().toObj().AutoDrop;
   assert.equal( autoDrop, false );

   db.getRecycleBin().alter( { AutoDrop: true } );
   var autoDrop = db.getRecycleBin().getDetail().toObj().AutoDrop;
   assert.equal( autoDrop, true );

   db.getRecycleBin().alter( { AutoDrop: false } );
   var autoDrop = db.getRecycleBin().getDetail().toObj().AutoDrop;
   assert.equal( autoDrop, false );

   db.getRecycleBin().setAttributes( { AutoDrop: true } );
   var autoDrop = db.getRecycleBin().getDetail().toObj().AutoDrop;
   assert.equal( autoDrop, true );

   db.getRecycleBin().setAttributes( { AutoDrop: false } );
   var autoDrop = db.getRecycleBin().getDetail().toObj().AutoDrop;
   assert.equal( autoDrop, false );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().alter( { AutoDrop: 0 } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().alter( { AutoDrop: 1 } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().alter( { AutoDrop: null } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().alter( { AutoDrop: "" } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().setAttributes( { AutoDrop: 0 } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().setAttributes( { AutoDrop: 1 } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().setAttributes( { AutoDrop: null } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().setAttributes( { AutoDrop: "" } );
   } );
}