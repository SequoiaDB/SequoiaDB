using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.monitor
{
    /**
     * description:  
     *                GetSnapshot（int snapType，BsonDocument matcher，BsonDocument selector，BsonDocument orderBy）
     *                SDBConst.SDB_SNAP_CONTEXTS
     *                SDBConst.SDB_SNAP_CONTEXTS_CURRENT
     *                SDBConst.SDB_SNAP_SESSIONS
     *                SDBConst.SDB_SNAP_SESSIONS_CURRENT
     *                SDBConst.SDB_SNAP_COLLECTIONS
     *                SDBConst.SDB_SNAP_COLLECTIONSPACES
     *                SDBConst.SDB_SNAP_DATABASE
     *                SDBConst.SDB_SNAP_SYSTEM
     *                SDBConst.SDB_SNAP_CATALOG
     *                SDBConst.SDB_SNAP_TRANSACTIONS
     *                SDBConst.SDB_SNAP_TRANSACTIONS_CURRENT
     *                SDBConst.SDB_SNAP_ACCESSPLANS
     *                SDBConst.SDB_SNAP_HEALTH
     *                1.创建cs.cl，并在cl中插入数据
     *                2.使用get_snapshot接口获取信息，分别验证如下场景：
     *                 a、只指定必填参数snapType
     *                 b、指定所有参数：matcher、selector、order_by
     *                 c、snapType覆盖所有值
     *                3、检查结果 
     * testcase:     seqDB-13622
     * author:       chensiqin
     * date:         2019/03/27
    */

    [TestClass]
    public class TestSnapshot13622
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string localCSName = "cs13622";
        private string clName = "cl13622";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();

        }

        [TestMethod]
        public void Test13622()
        {
            if ( Common.IsStandalone(sdb) == true)
            {
                return;
            }

            DBCursor cursor = null;
            BsonDocument doc = null;
            List<string> actual = new List<string>();

            //SDBConst.SDB_SNAP_CONTEXTS
            BsonDocument selector = new BsonDocument();
            selector.Add("Contexts", new BsonDocument("$include", 1));
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CONTEXTS, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("Contexts"));
            }
            cursor.Close();

            //SDBConst.SDB_SNAP_CONTEXTS_CURRENT
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CONTEXTS_CURRENT, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("Contexts"));
            }
            cursor.Close();

            //SDBConst.SDB_SNAP_SESSIONS
            selector = new BsonDocument();
            selector.Add("SessionID", new BsonDocument("$include", 1));
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SESSIONS, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("SessionID"));
            }
            cursor.Close();

            //SDBConst.SDB_SNAP_SESSIONS_CURRENT
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SESSIONS_CURRENT, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("SessionID"));
            }
            cursor.Close();

            //SDBConst.SDB_SNAP_COLLECTIONS
            cs = sdb.CreateCollectionSpace(localCSName);
            cs.CreateCollection(clName);

            string expectedStr = localCSName + "." + clName;
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", expectedStr);
            selector = new BsonDocument();
            selector.Add("Name", new BsonDocument("$include", 1));
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_COLLECTIONS, matcher, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
            }
            Assert.AreEqual(expectedStr, doc.GetElement("Name").Value.ToString());
            cursor.Close();

            //SDBConst.SDB_SNAP_COLLECTIONSPACES
            expectedStr = localCSName;
            matcher = new BsonDocument();
            matcher.Add("Name", expectedStr);
            selector = new BsonDocument();
            selector.Add("Name", new BsonDocument("$include", 1));
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_COLLECTIONSPACES, matcher, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
            }
            Assert.AreEqual(expectedStr, doc.GetElement("Name").Value.ToString());
            cursor.Close();

            //SDBConst.SDB_SNAP_DATABASE
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_DATABASE, null, null, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
            }
            Assert.AreEqual(true, doc.Contains("TotalNumConnects"));
            cursor.Close();

            //SDBConst.SDB_SNAP_SYSTEM
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SYSTEM, null, null, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
            }
            Assert.AreEqual(true, doc.Contains("CPU"));
            Assert.AreEqual(true, doc.Contains("Memory"));
            Assert.AreEqual(true, doc.Contains("Disk"));
            cursor.Close();

            //SDBConst.SDB_SNAP_CATALOG
            expectedStr = localCSName + "." + clName;
            matcher = new BsonDocument();
            matcher.Add("Name", expectedStr);
            selector = new BsonDocument();
            selector.Add("Name", new BsonDocument("$include", 1));
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            Assert.AreEqual(true, actual.Contains(expectedStr));
            cursor.Close();

            /*
              SDBConst.SDB_SNAP_TRANSACTIONS
              SDBConst.SDB_SNAP_TRANSACTIONS_CURRENT
             * */
            //SDBConst.SDB_SNAP_ACCESSPLANS //seqDB-14516 orderby
            //SDBConst.SDB_SNAP_HEALTH
            matcher = new BsonDocument();
            matcher.Add("IsPrimary", true);
            matcher.Add("Status", "Normal");
            selector = new BsonDocument();
            selector.Add("ServiceStatus", 1);
            selector.Add("IsPrimary", 1);
            BsonDocument hint = new BsonDocument();
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_HEALTH, matcher, selector, null);
            int num = 0;
            while (cursor.Next() != null)
            {
                num++;
                Assert.AreEqual("{ \"IsPrimary\" : true, \"ServiceStatus\" : true }", cursor.Current().ToString());
            }
            Assert.IsTrue(num>0);

            //SDB_LIST_SVCTASKS
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SVCTASKS, null, null, null, null, 0, 1);
            int count = 0;
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                count++;
                Assert.IsTrue(doc.ToString().Contains("\"TaskID\" : 0"));
                Assert.IsTrue(doc.ToString().Contains("\"TaskName\" : \"Default\""));  
            }
            cursor.Close();
            Assert.AreEqual(1, count);
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
