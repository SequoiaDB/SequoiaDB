/******************************************************************************
 * @Description   : seqDB-23831:Enable参数校验
 *                  seqDB-23848:开启/关闭回收站
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.21
 * @LastEditTime  : 2021.07.14
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main( test );
function test ()
{
   var recycleBin = db.getRecycleBin();
   assert.equal( recycleBin.getDetail().toObj().Enable, true );

   // recycleBin.alter
   recycleBin.alter( { "Enable": false } );
   assert.equal( recycleBin.getDetail().toObj().Enable, false );

   // recycleBin.setAttributes
   recycleBin.setAttributes( { "Enable": true } );
   assert.equal( recycleBin.getDetail().toObj().Enable, true );

   // recycleBin.disable
   recycleBin.disable();
   assert.equal( recycleBin.getDetail().toObj().Enable, false );

   // recycleBin.enable
   recycleBin.enable();
   assert.equal( recycleBin.getDetail().toObj().Enable, true );
}