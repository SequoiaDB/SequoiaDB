/* *****************************************************************************
@discretion: rename cs
             seqDB-16110 cs����domain�У��޸�cs����setDomain
@author��2018-10-13 chensiqin  Init
***************************************************************************** */

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var domainName = "domain16110";
   var csName1 = CHANGEDPREFIX + "_rename16110_1";
   var csName2 = CHANGEDPREFIX + "_rename16110_2";
   var clName = CHANGEDPREFIX + "renamecl16110";

   //����domain
   var groups = commGetGroups( db );
   var groupName1 = groups[0][0].GroupName;
   var groupName2 = groups[1][0].GroupName;
   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [groupName1, groupName2], { AutoSplit: true } );

   //����cs cl
   commDropCS( db, csName1, true, "ignoreNotExist is true" );
   commDropCS( db, csName2, true, "ignoreNotExist is true" );
   var varCS = commCreateCS( db, csName1, true, "create CS" );
   var varCL = commCreateCL( db, csName1, clName, { Group: groupName1 }, true, false, "create cl in the beginning" )
   insertData( varCL, 100 );

   testRenameCS16110( db, domainName, domain, csName1, csName2, clName );
   afterClear( db, domainName, csName2 )
}

function testRenameCS16110 ( db, domainName, domain, oldName, newName, clName )
{
   db.renameCS( oldName, newName );
   //check
   checkRenameCSResult( oldName, newName, 1 );
   var cs = db.getCS( newName )
   cs.setDomain( { Domain: domainName } )
   var csList = domain.listCollectionSpaces().toArray();
   var cs = eval( '(' + csList[0] + ')' );
   assert.equal( cs["Name"], newName );

   checkDatas( newName, clName );
}

function checkDatas ( csName2, clName )
{

   //check the record nums      
   var dbcl = db.getCS( csName2 ).getCL( clName );
   var count = dbcl.count();
   assert.equal( count, 100 );
}

function afterClear ( db, domainName, csName2 )
{
   commDropCS( db, csName2, true, "ignoreNotExist is true" );
   commDropDomain( db, domainName );
}