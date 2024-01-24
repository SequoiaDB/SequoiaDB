using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Meta
{
    /**
     * description:  createCollectionSpace (String csName, int pageSize) listCollectionSpaces ()；getCollectionSpace (String csName)； dropCollectionSpace (String csName)
     *              1、创建CS，指定pageSize参数，分别取值如下： 
     *                 a、所有范围内取值：SDB_PAGESIZE_4K/SDB_PAGESIZE_8K/SDB_PAGESIZE_16K/SDB_PAGESIZE_32K/SDB_PAGESIZE_64K/SDB_PAGESIZE_DEFAULT 
     *                 b、不在范围内取值，如取值为128K; 检查创建结果正确性； 
     *              2、查询所有CS（listCollectionSpaces），检查查询结果正确性； 
     *              3、删除CS，检查删除结果正确性（getCS（））； 
     * testcase:    14560
     * author:      chensiqin
     * date:        2018/05/02
    */

    [TestClass]
    public class TestCreateAndDropCS14560
    {

        private Sequoiadb sdb = null;
        private string csName1 = "csName14560_1";
        private string csName2 = "csName14560_2";
        private string csName3 = "csName14560_3";
        private string csName4 = "csName14560_4";
        private string csName5 = "csName14560_5";
        private string csName6 = "csName14560_6";
        private string csName7 = "csName14560_7";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14560()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            CreateCSAndCheck();
        }

        private void CreateCSAndCheck()
        {
            sdb.CreateCollectionSpace(csName1, SDBConst.SDB_PAGESIZE_4K);
            CheckCSPageSize(csName1, SDBConst.SDB_PAGESIZE_4K);

            sdb.CreateCollectionSpace(csName2, SDBConst.SDB_PAGESIZE_8K);
            CheckCSPageSize(csName2, SDBConst.SDB_PAGESIZE_8K);

            sdb.CreateCollectionSpace(csName3, SDBConst.SDB_PAGESIZE_16K);
            CheckCSPageSize(csName3, SDBConst.SDB_PAGESIZE_16K);

            sdb.CreateCollectionSpace(csName4, SDBConst.SDB_PAGESIZE_32K);
            CheckCSPageSize(csName4, SDBConst.SDB_PAGESIZE_32K);

            sdb.CreateCollectionSpace(csName5, SDBConst.SDB_PAGESIZE_64K);
            CheckCSPageSize(csName5, SDBConst.SDB_PAGESIZE_64K);

            sdb.CreateCollectionSpace(csName6, SDBConst.SDB_PAGESIZE_DEFAULT);
            CheckCSPageSize(csName6, 65536);

            DBCursor cur = sdb.ListCollectionSpaces();
            List<string> actual = new List<string>();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                BsonElement element = doc.GetElement("Name");
                actual.Add(element.Value.ToString());
            }
            cur.Close();
            Assert.IsTrue(actual.Contains(csName1));
            Assert.IsTrue(actual.Contains(csName2));
            Assert.IsTrue(actual.Contains(csName3));
            Assert.IsTrue(actual.Contains(csName4));
            Assert.IsTrue(actual.Contains(csName5));
            Assert.IsTrue(actual.Contains(csName6));

            sdb.DropCollectionSpace(csName1);
            sdb.DropCollectionSpace(csName2);
            sdb.DropCollectionSpace(csName3);
            sdb.DropCollectionSpace(csName4);
            sdb.DropCollectionSpace(csName5);
            sdb.DropCollectionSpace(csName6);

            try
            {
                sdb.GetCollectionSpace(csName1);
                Assert.Fail("expected thow BaseException but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-34, e.ErrorCode);
            }

            try
            {
                sdb.CreateCollectionSpace(csName7, 131072);
                Assert.Fail("expected thow BaseException but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
        }

        private void CheckCSPageSize(string ckCSName, int expectedPageSize)
        {
            DBQuery query = new DBQuery();
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            matcher.Add("Name", ckCSName);
            ReplicaGroup cataRg = sdb.GetReplicaGroup("SYSCatalogGroup");
            Sequoiadb cataDB = cataRg.GetMaster().Connect();
            CollectionSpace sysCS = cataDB.GetCollectionSpace("SYSCAT");
            DBCollection sysCL = sysCS.GetCollection("SYSCOLLECTIONSPACES");
            query.Matcher = matcher;
            DBCursor cur = sysCL.Query(query);
            Assert.IsNotNull(cur.Next());
            actual = cur.Current();
            Assert.AreEqual(expectedPageSize + "", actual.GetElement("PageSize").Value.ToString());
            cur.Close();
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
