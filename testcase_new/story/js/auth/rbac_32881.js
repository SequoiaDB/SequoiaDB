/******************************************************************************
 * @Description   : seqDB-32881:privilegecheck配置参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.08.31
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{

   // privilegecheck校验合法参数
   var config = { privilegecheck: true };
   updatePrivilegeCheck( config );

   var config = { privilegecheck: false };
   updatePrivilegeCheck( config );

   var config = { privilegecheck: "false" };
   updatePrivilegeCheck( config );

   var config = { privilegecheck: 1 };
   updatePrivilegeCheck( config );

   var config = { privilegecheck: "" };
   updatePrivilegeCheck( config );

   // privilegecheck校验非法参数
   var config = { privilegecheck: "aaa" };
   updatePrivilegeCheck( config );

   var config = { privilegecheck: -1 };
   updatePrivilegeCheck( config );

}

function updatePrivilegeCheck ( config )
{
   try
   {
      db.updateConf( config );
   }
   catch( e )
   {
      if( e != SDB_RTN_CONF_NOT_TAKE_EFFECT )
      {
         throw e;
      }
   }
}
