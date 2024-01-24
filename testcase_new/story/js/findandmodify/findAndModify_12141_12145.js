/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-7 wenjing wang  Init
*******************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var clName = COMMCLNAME + "_12141_12145";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   test_SetSessionAttrIsSlaveWithUpdate( db, cl );
   test_SetSessionAttrIsSlaveWithRemove( db, cl );
   commDropCL( db, COMMCSNAME, clName );
}
/*******************************************************************************
*@Description：测试op为update时, 设置setSessionAttr( {PreferedInstance:"S"} )
*@Input：find().update( {$inc:{a:1}} )
*@Expectation：更新在主节点上生效
********************************************************************************/
function test_SetSessionAttrIsSlaveWithUpdate ( db, cl )
{
   cl.insert( { a: 1 } );
   db.setSessionAttr( { PreferedInstance: "S" } );
   var arr = cl.find().update( { $inc: { a: 1 } } ).toArray();

   db.setSessionAttr( { PreferedInstance: "M" } );
   var recordnum = cl.find( { a: 2 } ).count();
   if( 1 != parseInt( recordnum ) )
   {
      throw new Error( "recordnum: " + recordnum );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 设置setSessionAttr( {PreferedInstance:"S"} )
*@Input：find( {a:1} ).remove()
*@Expectation：更新在主节点上生效
********************************************************************************/
function test_SetSessionAttrIsSlaveWithRemove ( db, cl )
{
   cl.insert( { a: 1 } );
   db.setSessionAttr( { PreferedInstance: "S" } );
   var arr = cl.find( { a: 1 } ).remove().toArray();

   db.setSessionAttr( { PreferedInstance: "M" } );
   var recordnum = cl.find( { a: 1 } ).count();
   if( 0 != parseInt( recordnum ) )
   {
      throw new Error( "recordnum: " + recordnum );
   }
   cl.truncate();
}
