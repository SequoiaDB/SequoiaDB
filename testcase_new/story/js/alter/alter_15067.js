/************************************
*@Description: 修改AutoSplit属性值
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15067
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
   if( 1 === allGroupName.length )
   {
      return;
   }
   var csName = CHANGEDPREFIX + "_cs_15067";
   var clName = CHANGEDPREFIX + "_cl_15067";
   var domainName = CHANGEDPREFIX + "_domain_15067";
   var group1 = allGroupName[0];
   var group2 = allGroupName[1];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [group1, group2], { AutoSplit: false } );
   db.createCS( csName, { Domain: domainName } )
   var clOption = { ShardingKey: { a: 1 }, ShardingType: 'hash' };
   var cl = commCreateCL( db, csName, clName, clOption, true, true );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   domain.setAttributes( { AutoSplit: true } )
   checkDomain( db, domainName, [group1, group2], true, undefined );

   checkCL( [group1, group2], csName, clName );

   db.dropCS( csName );
   commDropDomain( db, domainName );
}