/*******************************************************************
* @Description : list groups in domain
*                seqDB-14669:枚举域中的数据组
* @author      : Liang XueWang
*                2018-03-12
*******************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var groups = commGetDataGroupNames( db );
   var domainName = "testDomain14669";
   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, groups );

   var obj = domain.listGroups().next().toObj();

   // check domain name
   var name = obj["Name"];
   if( name !== domainName )
   {
      throw new Error( "expect: " + domainName + ", actual: " + name );
   }

   // check group num
   var groupArr = obj["Groups"];
   if( groupArr.length !== groups.length )
   {
      throw new Error( "expect: " + groups.length + ", actual: " + groupArr.length );
   }

   // check groups name
   for( var i = 0; i < groups.length; i++ )
   {
      if( groupArr[i]["GroupName"] !== groups[i] )
      {
         throw new Error( "expect: " + groups[i] + ", actual: " + groupArr[i]["GroupName"] );
      }
   }

   commDropDomain( db, domainName );
}
