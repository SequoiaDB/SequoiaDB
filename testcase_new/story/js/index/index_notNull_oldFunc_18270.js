/******************************************************************************
*@Description : seqDB-18270:原有接口基本功能验证 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/


main( test );

function test ()
{
   var clName = "cl_18270";
   var indexName = "idx";

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );

   var unique = true;
   var enforced = true;
   var sortBufferSize = 32;
   cl.createIndex( indexName, { a: 1 }, true, true, sortBufferSize );

   var NotNull = false;
   checkIndex( cl, indexName, unique, enforced, NotNull );

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false );
}

function checkIndex ( cl, indexName, expUni, expEnf, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actUni = indexDef.unique;
   var actEnf = indexDef.enforced;
   var actNot = indexDef.NotNull;
   if( actUni !== expUni || actEnf !== expEnf || actNot !== expNot )
   {
      var expResults = JSON.stringify( { unique: expUni, enforced: expEnf, NotNull: expNot } );
      var actResults = JSON.stringify( { unique: actUni, enforced: actEnf, NotNull: actNot } );
      throw new Error( "checkResult fail," + expResults + "  " + actResults );
   }
}