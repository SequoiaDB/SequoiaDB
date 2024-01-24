/******************************************************************************
 * @Description   : seqDB-23701:sequences相关接口测试
 * @Author        : Yu Fan
 * @CreateTime    : 2021.03.18
 * @LastEditTime  : 2021.03.18
 * @LastEditors   : Yu Fan
 ******************************************************************************/
var seq_name = "seq_23701";
var seq_name_new = "seq_23701_new";
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   // 清理环境
   clearEnv();

   // 创建序列
   var options = {
      StartValue: 10, MinValue: 10, MaxValue: 23701, Increment: 10,
      CacheSize: 1, AcquireSize: 1, Cycled: true
   };
   tryCatch( ["cmd=create sequence", "name=" + seq_name, "options=" + JSON.stringify( options )], [0] );

   // 获取sequence快照
   var exp = {
      StartValue: 10, MinValue: 10, MaxValue: 23701, Increment: 10,
      CacheSize: 1, AcquireSize: 1, Cycled: true, CurrentValue: 10
   };
   snapshotSequence( exp );

   // 列取sequence
   listSequences();

   // 获取sequence 下一个值
   tryCatch( ["cmd=get sequence next value", "name=" + seq_name], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).NextValue, options.MinValue, infoSplit );

   // 获取sequence当前值
   tryCatch( ["cmd=get sequence current value", "name=" + seq_name], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).CurrentValue, options.MinValue, infoSplit );

   // 重命名sequence
   tryCatch( ["cmd=rename sequence", "name=" + seq_name, "newname=" + seq_name_new], [0] );
   // 获取sequence 下一个值，检查结果
   tryCatch( ["cmd=get sequence next value", "name=" + seq_name_new], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).NextValue, options.MinValue + options.Increment, infoSplit );

   // 重置sequence 计数
   var startValue = 11;
   tryCatch( ["cmd=restart sequence", "name=" + seq_name_new, "StartValue=" + startValue], [0] );
   // 获取sequence 下一个值，检查结果
   tryCatch( ["cmd=get sequence next value", "name=" + seq_name_new], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).NextValue, startValue, infoSplit );


   // 修改 sequence 属性
   var modifiedOptions = { MinValue: 1, MaxValue: 10 };
   tryCatch( ["cmd=set sequence attributes", "name=" + seq_name_new, "options=" + JSON.stringify( modifiedOptions )], [0] );
   // 获取快照，检查结果
   var filter = { Name: seq_name_new };
   var selector = { "MaxValue": 1, "MinValue": 1 }
   tryCatch( ["cmd=snapshot sequences", "filter=" + JSON.stringify( filter ), "selector=" + JSON.stringify( selector )], [0] );
   var act = infoSplit[1];
   assert.equal( JSON.parse( act ), modifiedOptions, infoSplit )

   // 删除sequence
   tryCatch( ["cmd=drop sequence", "name=" + seq_name_new], [0] );
   // 获取sequence 下一个值，检查结果
   tryCatch( ["cmd=get sequence next value", "name=" + seq_name_new], [SDB_SEQUENCE_NOT_EXIST] );
}

function listSequences ()
{
   tryCatch( ["cmd=list sequences"], [0] );
   resp = infoSplit;
   assert.equal( resp.length > 1, true );
   var doesExist = false;
   for( var i = 1; i < resp.length; i++ )
   {
      if( JSON.parse( resp[i] ).Name == seq_name )
      {
         doesExist = true;
         break;
      }

   }
   assert.equal( doesExist, true );
}


function snapshotSequence ( exp )
{
   var filter = { Name: seq_name };
   var selector = {
      "AcquireSize": 1, "CacheSize": 1, "CurrentValue": 1, "Cycled": 1,
      "Increment": 1, "MaxValue": 1, "MinValue": 1, "StartValue": 1
   }
   tryCatch( ["cmd=snapshot sequences", "filter=" + JSON.stringify( filter ), "selector=" + JSON.stringify( selector )], [0] );
   var act = infoSplit[1];
   assert.equal( JSON.parse( act ), exp )
}


function clearEnv ()
{
   try
   {
      db.dropSequence( seq_name );
   } catch( e )
   {
      if( e != SDB_SEQUENCE_NOT_EXIST )
      {
         throw new Error( e );
      }
   }

   try
   {
      db.dropSequence( seq_name_new );
   } catch( e )
   {
      if( e != SDB_SEQUENCE_NOT_EXIST )
      {
         throw new Error( e );
      }
   }
}

