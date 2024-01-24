/******************************************************************************
 * @Description   : 1. Test db.listDomains(<name>,[option]), specify name list.
                    2. Test insert/update/find/remove operation.
                    3. Test create four domains.
 * @Author        : xiaojun Hu
 * @CreateTime    : 2014.06.18
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : Lai Xuan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domNames = new Array( CHANGEDPREFIX + "DomTest1_8128", CHANGEDPREFIX + "DomTest2_8128",
      CHANGEDPREFIX + "DomTest3_8128", CHANGEDPREFIX + "DomTest4_8128" );

   // Drop all domains, if have
   for( var i = 0; i < domNames.length; ++i )
   {
      clearDomain( db, domNames[i] );
   }

   var group = getGroup( db );
   for( var i = 0; i < domNames.length; ++i )
   {
      db.createDomain( domNames[i], group );
   }

   // Inspect the specify domain
   for( var j = 0; j < domNames.length; ++j )
   {
      var flag = false; // 确认是否存在特定域
      var listDom = db.listDomains( { "Name": domNames[j] } ).current().toObj()["Name"];
      if( listDom === domNames[j] )
      {
         flag = true;
      }
      assert.equal( flag, true, "ErrDomains" );
   }



   for( var i = 0; i < domNames.length; ++i )
   {
      clearDomain( db, domNames[i] );
   }
}