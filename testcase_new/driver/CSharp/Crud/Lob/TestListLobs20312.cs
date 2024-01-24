using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Lob
{
    /**
    * description:   1、创建集合，并写入大小不一的lob
     *                2、listLobs查询lob，分别测试以下条件：query/selector/hint/orderby/skip/limit
     *                3、检查结果
    * testcase:      20312
    * author:        chensiqin
    * date:          2019/12/12
    */
    [TestClass]
    public class TestListLobs20312
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl20312";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
        }

        [TestMethod]
        public void Test20312()
        {
            putLob();
            listLob();
            ObjectId cc = cl.CreateLobID();
            Console.WriteLine(cc.ToString());
            DateTime dt = new DateTime();
            ObjectId dd = cl.CreateLobID( dt );

        }

        public void listLob()
        {
            try
            {
                BsonDocument matcher = new BsonDocument();
                BsonDocument selector = new BsonDocument();
                BsonDocument orderBy = new BsonDocument();
                BsonDocument hint = new BsonDocument();
                matcher.Add("Size", new BsonDocument("$gt",100));
                selector.Add("Size", 1);
                orderBy.Add("Size", -1);
                DBCursor cursor = cl.ListLobs(matcher, selector, orderBy, hint, 1, 1);
                int count=0;
                while (cursor.Next() != null)
                {
                    count++;
                    Assert.AreEqual("{ \"Size\" : 300 }", cursor.Current().ToString());
                }
                Assert.AreEqual(1, count);
           }
            catch (BaseException e)
            {
                Assert.Fail("listLob fail:" + e.Message + e.ErrorCode);
            }
        }

        private void putLob()
        {
            int[] lobsize = {100, 200, 300, 400};
            try
            {
                for (int i = 0; i < 4; i++)
                {
                    byte[] testLobBuff = LobUtils.GetRandomBytes(lobsize[i]);
                    DBLob lob = cl.CreateLob();
                    lob.Write(testLobBuff);
                    ObjectId oid = lob.GetID();
                    lob.Close();
                }
            }
            catch (BaseException e)
            {
                Assert.Fail("write lob fail" + e.Message);
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }
    }
}
