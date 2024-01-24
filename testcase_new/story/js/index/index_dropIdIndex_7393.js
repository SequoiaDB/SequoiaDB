/************************************************************************
@Description : drop the index "$id".Cannot drop .
@Modify list :
               2014-5-16  xiaojun Hu  modify
************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   assert.tryThrow( SDB_IXM_DROP_ID, function()
   {
      varCL.dropIndex( "$id" );
   } );

   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false );
}

