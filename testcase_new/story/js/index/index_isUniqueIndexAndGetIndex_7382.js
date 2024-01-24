/****************************************************************************
@Description : Creating index and get the index .
@Modify list :
               2014-5-18  xiaojun Hu  Modify
****************************************************************************/

main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
   var varCL = commCreateCL( db, csName, clName );

   varCL.createIndex( "testindex", { a: 1 }, true );
   varCL.createIndex( "nameIndex", { "name": -1 }, true, false );

   varCL.insert( { a: 1, "name": "hihao" } );

   var index = varCL.getIndex( "testindex" );

   index = index.toString();
   index = eval( '(' + index + ')' );
   var _index = index["IndexDef"];
   //var _index = eval("("+_index+")") ;
   assert.equal( "testindex", _index["name"] );

   //_index = eval("("+index["key"]+")") ;
   assert.equal( 1, _index["key"]["a"] );
   assert.equal( true, _index["unique"] );

   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      varCL.insert( { a: 1, "name": "hihao1232" } );
   } );

   commDropCL( db, csName, clName, false, false );

}
