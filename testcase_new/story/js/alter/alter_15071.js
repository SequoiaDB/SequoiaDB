/************************************
*@Description: alter修改domain所有属性值
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15071
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 2 >= allGroupName.length )
   {
      return;
   }
   var csName = CHANGEDPREFIX + "_cs_15071";
   var clName = CHANGEDPREFIX + "_cl_15071";
   var domainName = CHANGEDPREFIX + "_domain_15071";
   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   var group3 = allGroupName[2];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [group1], { AutoSplit: false } );
   db.createCS( csName, { Domain: domainName } )
   var clOption = { ShardingKey: { a: 1 }, ShardingType: 'hash' };
   var cl = commCreateCL( db, csName, clName, clOption, true, true );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   domain.alter( { Groups: [group1, group2, group3], AutoSplit: true, AutoRebalance: true } )
   checkDomain( db, domainName, [group1, group2, group3], true, true );

   domain.alter( {
      Alter: [{ Name: "set attributes", Args: { AutoSplit: false } },
      { Name: "set attributes", Args: { Name: 'test_15071' } },
      { Name: "addgroups", Args: { Groups: [group1, group2, group3] } }],
      Options: { IgnoreException: true }
   } )
   checkDomain( db, domainName, [group1, group2, group3], false, true );

   db.dropCS( csName );
   commDropDomain( db, domainName );
}

function checkDomain ( db, domainName, expGroups, expAutoSplit, expAutoRebalance )
{
   var domainMsg = db.listDomains( { Name: domainName } ).current().toObj();
   actGroups = domainMsg.Groups;
   actAutoSplit = domainMsg.AutoSplit;
   actAutoRebalance = domainMsg.AutoRebalance;

   assert.equal( actGroups.length, expGroups.length );
   for( var i in expGroups )
   {
      var groupName = actGroups[i].GroupName;
      assert.notEqual( expGroups.indexOf( groupName ), -1 );
   }

   assert.equal( actAutoSplit, expAutoSplit );
   assert.equal( actAutoRebalance, expAutoRebalance );
}