using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.AutoIncrement
{
    /**
     * TestCase : seqDB-16636
     * test interface:   sequence快照/列表验证 
     *                   GetSnapshot (int snapType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy)
     *                   GetSnapshot (int snapType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy, BsonDocument hint)
     *                   GetSnapshot (int snapType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy, BsonDocument hint, long skipRows, long returnRows)
     *                   GetList (int listType)
     *                   GetList (int listType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy)
     *                   
     * author:  chensiqin
     * date:    2018/12/12
     * version: 1.0
    */

    [TestClass]
    public class AutoIncrement16636
    {

        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string clName = "cl16636";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test16636()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }

            CreatAutoIncreamentCL();

            TestGetSnapshot();

            TestGetList();

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

        public void CreatAutoIncreamentCL()
        {
            DBCollection cl = cs.CreateCollection(clName);
            List<BsonDocument> optionList = new List<BsonDocument>();

            BsonDocument options = new BsonDocument();
            options.Add("Field", "test16636_1");
            options.Add("MinValue", 1);
            options.Add("StartValue", 2);
            optionList.Add(options);

            options = new BsonDocument();
            options.Add("Field", "test16636_2");
            options.Add("MinValue", 1);
            options.Add("StartValue", 4);
            optionList.Add(options);

            options = new BsonDocument();
            options.Add("Field", "test16636_3");
            options.Add("MinValue", 3);
            options.Add("StartValue", 6);
            optionList.Add(options);

            cl.CreateAutoIncrement(optionList);

        }

        public void TestGetSnapshot()
        {
            List<BsonDocument> autoIncrementInfos = GetAutoIncrement(SdbTestBase.csName + "." + clName);

            BsonDocument doc = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            BsonDocument orderBy = new BsonDocument();
            BsonDocument matcher = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(new BsonDocument { { "Name", autoIncrementInfos[0].GetElement("SequenceName").Value.ToString() } });
            arr.Add(new BsonDocument { { "Name", autoIncrementInfos[1].GetElement("SequenceName").Value.ToString() } });
            arr.Add(new BsonDocument { { "Name", autoIncrementInfos[2].GetElement("SequenceName").Value.ToString() } });
            matcher.Add("$or", arr);
            selector.Add("StartValue", 1);
            orderBy.Add("StartValue", -1);

            //GetSnapshot (int snapType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy)
            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, matcher, selector, orderBy);
            List<BsonDocument> expected = new List<BsonDocument>();
            List<BsonDocument> actual = new List<BsonDocument>();
            expected.Add(new BsonDocument { { "StartValue", 6 } });
            expected.Add(new BsonDocument { { "StartValue", 4 } });
            expected.Add(new BsonDocument { { "StartValue", 2 } });
            while (cur.Next() != null)
            {
                doc = cur.Current();
                actual.Add(doc);
            }
            cur.Close();
            Assert.AreEqual(expected.ToString(), actual.ToString());

            // GetSnapshot (int snapType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy, BsonDocument hint)
            BsonDocument hint = new BsonDocument();
            cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, matcher, selector, new BsonDocument { { "StartValue", 1 } }, hint);
            expected = new List<BsonDocument>();
            actual = new List<BsonDocument>();
            expected.Add(new BsonDocument { { "StartValue", 2 } });
            expected.Add(new BsonDocument { { "StartValue", 4 } });
            expected.Add(new BsonDocument { { "StartValue", 6 } });
            while (cur.Next() != null)
            {
                doc = cur.Current();
                actual.Add(doc);
            }
            cur.Close();
            Assert.AreEqual(expected.ToString(), actual.ToString());

            // GetSnapshot (int snapType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy, BsonDocument hint, long skipRows, long returnRows)
            long skipRows = 1;
            long returnRows = 1;
            cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, matcher, selector, orderBy, hint, skipRows, returnRows);
            expected = new List<BsonDocument>();
            actual = new List<BsonDocument>();
            expected.Add(new BsonDocument { { "StartValue", 4 } });
            while (cur.Next() != null)
            {
                doc = cur.Current();
                actual.Add(doc);
            }
            cur.Close();
            Assert.AreEqual(expected.ToString(), actual.ToString());
        }

        public void TestGetList()
        {
            // GetList (int listType)
            List<BsonDocument> autoIncrementInfos = GetAutoIncrement(SdbTestBase.csName + "." + clName);
            List<string> sequenceNames = new List<string>();
            sequenceNames.Add(autoIncrementInfos[0].GetElement("SequenceName").Value.ToString());
            sequenceNames.Add(autoIncrementInfos[1].GetElement("SequenceName").Value.ToString());
            sequenceNames.Add(autoIncrementInfos[2].GetElement("SequenceName").Value.ToString());

            DBCursor cur = sdb.GetList(SDBConst.SDB_SNAP_SEQUENCES);
            int expected = 3;
            int actual = 0;
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Console.WriteLine(doc.ToString());
                if (sequenceNames.Contains(doc.GetElement("Name").Value.ToString()))
                {
                    actual++;
                }
            }
            if (actual < expected)
            {
                Assert.Fail("expected >= 3 actual : " + actual);
            }
            cur.Close();

            //GetList (int listType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy)
            cur = sdb.GetList(SDBConst.SDB_SNAP_SEQUENCES, null, null, null);
            expected = 3;
            actual = 0;
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Console.WriteLine(doc.ToString());
                if (sequenceNames.Contains(doc.GetElement("Name").Value.ToString()))
                {
                    actual++;
                }
            }
            if (actual < expected)
            {
                Assert.Fail("expected >= 3 actual : " + actual);
            }
            cur.Close();

        }

        public List<BsonDocument> GetAutoIncrement(string clFullName)
        {
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", clFullName);
            BsonDocument selector = new BsonDocument();
            selector.Add("AutoIncrement", 1);
            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, selector, null);
            List<BsonDocument> doc = new List<BsonDocument>();
            while (cur.Next() != null)
            {
                BsonElement element = cur.Current().GetElement("AutoIncrement");
                BsonArray arr = element.Value.AsBsonArray;
                doc.Add((BsonDocument)arr[0]);
                doc.Add((BsonDocument)arr[1]);
                doc.Add((BsonDocument)arr[2]);
            }
            cur.Close();
            return doc;
        }
    }
}
