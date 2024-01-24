/******************************************************************************
 * @Description   : seqDB-26406:catalog切主后获取序列值
 * @Author        : liuli
 * @CreateTime    : 2022.04.20
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var sequenceName = 'seq_26406';
   dropSequence( db, sequenceName );
   var seq = db.createSequence( sequenceName );

   // 获取序列值
   seq.fetch( 1000 );

   // 新建一个连接
   db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var rg = db2.getCataRG();
   rg.reelect();

   // 再次获取序列
   seq.fetch( 1000 );
   commCheckBusinessStatus( db );

   db2.close();
   db.dropSequence( sequenceName );
}