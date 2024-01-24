using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Sdb
{
    /**
     * description:  
     *                 1、执行大量查询操作（循环多次查询）
     *                 2、检视驱动代码游标是否正常关闭
     * testcase:     seqDB-14650
     * author:       chensiqin
     * date:         2019/03/29
    */

    [TestClass]//TODO:文本用例中步骤动作有两步，期望结果应该也分为两步吧？期望结果中的游标是我们自己关闭的，不是自动关闭
    public class TestDBCursor14650
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14650";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14650()
        {

            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            cl.Insert(new BsonDocument("name",14650));
            DBCursor cursor = null;
            for (int i = 0; i < 200; i++)
            {
                cursor = cl.Query();
                while (cursor.Next() != null)
                {
                    cursor.Current();
                }
                cursor.Close();
            }

            try
            {
                cursor.Next();
                Assert.Fail("expected failed but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-31, e.ErrorCode);
            }
          
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
