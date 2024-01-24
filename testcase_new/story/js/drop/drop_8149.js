/******************************************************************************
*@Description : seqDB-8149:dropCS参数校验(1)
               1. 指定多个参数删除CS
               2. 删除名称以 $ 开头的CS 
               3. 删除名称中包含 . 的CS
               4. 删除名称为空的CS
               5. 删除名称长度为 128 字节的CS
               6. dropCS,检查listCollectionSpaces结果是否正确
               7. 再次删除已经删除的CS
*@author:      liyuanyue
*@createdate:  2020.07.17
******************************************************************************/
testConf.csName = COMMCLNAME + "_8149";
main( test );

function test ( testPara )
{
   var csName = COMMCLNAME + "_8149";

   /* SEQUOIADBMAINSTREAM-5174
      待开发确认接口是否做参数校验后放开或删除
   var csName1 = COMMCSNAME + "_8149_1";
   var csName2 = COMMCSNAME + "_8149_2";

   // 指定多个参数删除CS
   commDropCS( db, csName1 );
   commDropCS( db, csName2 );

   commCreateCS( db, csName1 );
   commCreateCS( db, csName2 );

   try
   {
      db.dropCS( csName1, csName2 );
      throw new Error( "Drop multiple cs incorectlly" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   */

   // 删除名称以 $ 开头的CS
   var name = "$" + csName;
   var errno = SDB_INVALIDARG;
   var message = "error,dropCS start $ cs succeeded";
   illegaldropCS( name, errno, message );

   // 删除名称中包含 . 的CS
   var name = csName + "." + csName;
   var errno = SDB_INVALIDARG;
   var message = "error,dropCS contain . cs succeeded";
   illegaldropCS( name, errno, message );

   // 删除名称为空的CS
   var name = "";
   var errno = SDB_INVALIDARG;
   var message = "error,dropCS empty cs succeeded";
   illegaldropCS( name, errno, message );

   // 删除名称长度为 128 字节的CS
   var name = new Array( 129 ).join( 'c' );
   var errno = SDB_INVALIDARG;
   var message = "error,dropCS 128 byte cs succeeded";
   illegaldropCS( name, errno, message );

   // dropCS, 检查list结果是否正确
   db.dropCS( csName );
   var cond = { Name: csName };
   var cur = db.list( 5, cond );
   if( cur.size() != 0 )
   {
      throw new Error( "have droped the cs:" + csName + ", but it still exist" );
   }

   // 再次删除已经删除的CS
   var name = csName;
   var errno = SDB_DMS_CS_NOTEXIST;
   var message = "have droped the cs:" + csName + ", but it can be dropCS again";
   illegaldropCS( name, errno, message );
}

function illegaldropCS ( csName, errno, message )
{
   try
   {
      db.dropCS( csName );
      throw new Error( message );
   } catch( e )
   {
      if( e.message != errno )
      {
         throw e;
      }
   }
}
