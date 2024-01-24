/***************************************************************************************************
 * @Description: 设置集合同步一致性策略
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: JiangFeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 12/08/2022 JiangFeng You Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    验证集合同步一致性策略
 * 测试步骤：
 *    1. 设置集合同步一致性策略
 *    2. 设置集合同步一致性策略非法值
 * 期望结果：
 *    步骤2：非法值报错，-6
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_consistency_1";

main(test);
function test(args)
{
   var cl = args.testCL ;
   cl.alter( { "ConsistencyStrategy":1 } ) ;
   cl.alter( { "ConsistencyStrategy":2 } ) ;
   cl.alter( { "ConsistencyStrategy":3 } ) ;

   //alter cl
   try
   {
      cl.alter( { "ConsistencyStrategy": true } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.alter( { "ConsistencyStrategy": [111] } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.alter( { "ConsistencyStrategy": {"bson":"1"} } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.alter( { "ConsistencyStrategy": "string" } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.alter( { "ConsistencyStrategy": 0 } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.alter( { "ConsistencyStrategy": 4 } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   cl.setAttributes( { "ConsistencyStrategy":1 } ) ;
   cl.setAttributes( { "ConsistencyStrategy":2 } ) ;
   cl.setAttributes( { "ConsistencyStrategy":3 } ) ;

   //alter cl
   try
   {
      cl.setAttributes( { "ConsistencyStrategy": true } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.setAttributes( { "ConsistencyStrategy": [111] } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.setAttributes( { "ConsistencyStrategy": {"bson":"1"} } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.setAttributes( { "ConsistencyStrategy": "string" } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.setAttributes( { "ConsistencyStrategy": 0 } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   //alter cl
   try
   {
      cl.setAttributes( { "ConsistencyStrategy": 4 } );
      throw new Error( "ERR" ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

}

