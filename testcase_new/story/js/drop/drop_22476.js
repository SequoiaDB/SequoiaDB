/******************************************************************************
*@Description : seqDB-22476:dropCS新增参数EnsureEmpty
               1、直连coord节点dropCS，EnsureEmpt参数取值覆盖：
                  1）有效取值：
                     true：cs下存在集合、cs下不存在集合
                     false：cs下存在集合、cs下不存在集合
                  2）无效取值：
                     任意数字、字符串、null
                  3）不指定EnsureEmpty（默认为false）：cs下存在集合、cs下不存在集合
               2、直连data节点dropCS，EnsureEmpt参数取值覆盖：   
                  true：cs下存在集合，cs下不存在集合
                  false：cs下存在集合
*@author:      liyuanyue
*@createdate:  2020.07.16
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var csName = COMMCSNAME + "_22476";
   var clName = COMMCLNAME + "_22476";

   commDropCS( db, csName );

   // 直连 coord 节点
   // { EnsureEmpty: true }; cs 下不存在 cl
   var option = { EnsureEmpty: true };
   commCreateCS( db, csName );
   db.dropCS( csName, option );
   var expResult = false;
   checkCSExists( db, csName, expResult );

   // { EnsureEmpty: true }; cs 下存在 cl
   var cs = commCreateCS( db, csName );
   cs.createCL( clName );
   dropCSCsNotEmpty( db, csName, option );
   var expResult = true;
   checkCSExists( db, csName, expResult );

   commDropCS( db, csName );

   // { EnsureEmpty: false }; cs 下不存在 cl
   var option = { EnsureEmpty: false };
   commCreateCS( db, csName );
   db.dropCS( csName, option );
   var expResult = false;
   checkCSExists( db, csName, expResult );

   // { EnsureEmpty: false }; cs 下存在 cl
   var cs = commCreateCS( db, csName );
   cs.createCL( clName );
   db.dropCS( csName, option );
   var expResult = false;
   checkCSExists( db, csName, expResult );

   // 检查无效值
   var option = { EnsureEmpty: 2 };
   dropCSInvalidValue( db, csName, option );
   var option = { EnsureEmpty: "wpqroafazc" };
   dropCSInvalidValue( db, csName, option );
   var option = { EnsureEmpty: "null" };
   dropCSInvalidValue( db, csName, option );

   // 检查默认值
   // 不指定 option;cs 下存在 cl
   var cs = commCreateCS( db, csName );
   cs.createCL( clName );
   db.dropCS( csName );
   var expResult = false;
   checkCSExists( db, csName, expResult );

   // 直连data节点
   var data = db.getRG( groupName ).getMaster().connect();

   commDropCS( data, csName );

   // { EnsureEmpty: true }; cs下不存在cl
   var option = { EnsureEmpty: true };
   commCreateCS( data, csName );
   data.dropCS( csName, option );
   var expResult = false;
   checkCSExists( data, csName, expResult );

   // { EnsureEmpty: true };cs下存在cl
   var cs = commCreateCS( data, csName );
   cs.createCL( clName );
   dropCSCsNotEmpty( data, csName, option );
   var expResult = true;
   checkCSExists( data, csName, expResult );

   commDropCS( data, csName );

   //  { EnsureEmpty: false }; cs下存在cl
   var option = { EnsureEmpty: false };
   var cs = commCreateCS( data, csName );
   cs.createCL( clName );
   data.dropCS( csName, option );

   var expResult = false;
   checkCSExists( db, csName, expResult );
}

function checkCSExists ( db, csName, expResult )
{
   var actResult = false;
   var cond = { Name: csName };
   var cur = db.list( 5, cond );
   if( cur.size() == 1 )
   {
      actResult = true;
   }
   if( expResult !== actResult )
   {
      throw new Error( "check CS exists,expect Result:"
         + expResult + ",but actually Result:" + actResult );
   }
}

function dropCSInvalidValue ( db, csName, option )
{
   try
   {
      db.dropCS( csName, option );
      throw new Error( "error,dropCS with " + JSON.stringify( option ) + " successed" );
   } catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}
function dropCSCsNotEmpty ( db, csName, option )
{
   try
   {
      db.dropCS( csName, option );
      throw new Error( "error,cs contains cl dropCS with EnsureEmpty:true successed" );
   } catch( e )
   {
      if( e.message != SDB_DMS_CS_NOT_EMPTY )
      {
         throw e;
      }
   }
}
