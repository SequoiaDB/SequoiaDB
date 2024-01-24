/******************************************************************************
 * @Description   : seqDB-13730:游标未关闭时，在不同个session中删除cs
 * @Author        : xiaojunHu
 * @CreateTime    : 2014.09.26
 * @LastEditTime  : 2022.05.17
 * @LastEditors   : liuli
 ******************************************************************************/

testConf.csName = COMMCSNAME + "_13730";
testConf.clName = COMMCLNAME + "_13730";

main( test );
function test ( testPara )
{
   var rd = new commDataGenerator();
   var recs = rd.getRecords( 50000, "string", ['a', 'b', 'c'] );
   testPara.testCL.insert( recs );

   //cursor not close
   try
   {
      var rc = testPara.testCL.find();
      rc.next();
      dropcsDiffSession( testConf.csName );
      assert.tryThrow( SDB_RTN_CONTEXT_NOTEXIST, function()
      {
         while( rc.next() ) { }
      } );
   }
   finally
   {
      rc.close();
   }
}

function dropcsDiffSession ( csName )
{
   var dbAnother = null;
   try
   {
      dbAnother = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      dbAnother.dropCS( csName );
      assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
      {
         dbAnother.getCS( csName );
      } );

   }
   finally
   {
      if( dbAnother != null )
      {
         dbAnother.close();
      }
   }
}