/******************************************************************************
 * @Description   : seqDB-23242:indexcoveron动态生效
 * @Author        : Yi Pan
 * @CreateTime    : 2021.01.19
 * @LastEditTime  : 2021.01.19
 * @LastEditors   : Yi Pan
 ******************************************************************************/

main( test );

function test ()
{
   //snapshot检查默认
   checkIndexcoveron( "TRUE" )

   try
   {    //修改配置文件为false snapshot检查
      db.updateConf( { indexcoveron: false }, { Global: true } );
      checkIndexcoveron( "FALSE" )
   } finally
   {
      db.updateConf( { indexcoveron: true }, { Global: true } );
   }

   //修改配置文件为true snapshot检查
   db.updateConf( { indexcoveron: true }, { Global: true } );
   checkIndexcoveron( "TRUE" )

}

//检查checkIndexcoveron为"TRUE"还是"FALSE"
function checkIndexcoveron ( flag )
{
   snapshot = db.snapshot( 13 );
   while( snapshot.next() )
   {
      var actResult = snapshot.current().toObj().indexcoveron;
      assert.equal( actResult, flag );
   }
}