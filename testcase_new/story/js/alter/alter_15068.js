/************************************
*@Description: 修改AutoSplit属性值
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15068
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
   var csName = CHANGEDPREFIX + "_cs_15068";
   var clName = CHANGEDPREFIX + "_cl_15068";
   var domainName = CHANGEDPREFIX + "_domain_15068";
   var group1 = allGroupName[0];
   var group2 = allGroupName[1];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [group1, group2], { AutoSplit: true } );

   domain.setAttributes( { AutoSplit: false } )
   checkDomain( db, domainName, [group1, group2], false, undefined );

   db.createCS( csName, { Domain: domainName } )
   var clOption = { ShardingKey: { a: 1 }, ShardingType: 'hash' };
   var cl = commCreateCL( db, csName, clName, clOption, true, true );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }
   checkCL( [group1, group2], csName, clName );

   db.dropCS( csName );
   commDropDomain( db, domainName );
}