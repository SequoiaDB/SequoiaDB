using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Crud.Analyze
{
    /**
     * description: analyze group
     * testcase:    14260
     * author:      linsuqiang
     * date:        2018/07/31
     */

    [TestClass]
    public class AnalyzeGroup14260
    {
        private Sequoiadb sdb = null;
        private const string csName = "analyze14260";
        private const string clName = "analyze14260";
        private DBCollection cl = null;
        private string analyzeGroup = null;
        private string nonAnalyzeGroup = null;
        private BsonDocument analyzeRec = null;
        private BsonDocument nonAnalyzeRec = null;
        private bool skipTest = false;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            if (Common.IsStandalone(sdb) || Common.getDataGroupNames(sdb).Count < 2)
            {
                Console.WriteLine("no group enough. skip this test");
                skipTest = true;
                return;
            }
            if (sdb.IsCollectionSpaceExist(csName))
            {
                sdb.DropCollectionSpace(csName);
            }
            CollectionSpace cs = sdb.CreateCollectionSpace(csName);
            List<string> groupNames = Common.getDataGroupNames(sdb);
            analyzeGroup = groupNames.ElementAt(0);
            nonAnalyzeGroup = groupNames.ElementAt(1);
            BsonDocument options = BsonDocument.Parse("{ShardingKey:{a: 1}, ShardingType:'range'}");
            options.Add("Group", analyzeGroup);
            cl = cs.CreateCollection(clName, options);
            cl.Split(analyzeGroup, nonAnalyzeGroup, new BsonDocument("a", 1000), new BsonDocument("a", 3000));

            analyzeRec = new BsonDocument("a", 0); // record on analyzeGroup
            InsertData(cl, analyzeRec);
            nonAnalyzeRec = new BsonDocument("a", 2000); // record on nonAnalyzeGroup
            InsertData(cl, nonAnalyzeRec);
        }

        [TestMethod]
        public void Test14260()
        {
            if (skipTest) return;
            CheckScanTypeByExplain(cl, analyzeRec, "ixscan");
            CheckScanTypeByExplain(cl, nonAnalyzeRec, "ixscan");
            sdb.Analyze(new BsonDocument{ {"GroupName", analyzeGroup} });
            CheckScanTypeByExplain(cl, analyzeRec, "ixscan");
            CheckScanTypeByExplain(cl, nonAnalyzeRec, "ixscan");
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }

        private void InsertData(DBCollection cl, BsonDocument rec)
        {
            List<BsonDocument> recs = new List<BsonDocument>();
            int recNum = 2000;
            for (int i = 0; i < recNum; ++i)
            {
                recs.Add(new BsonDocument(rec));
            }
            cl.BulkInsert(recs, 0);
        }

        private void CheckScanTypeByExplain(DBCollection cl, BsonDocument cond, string expScanType)
        {
            BsonDocument options = new BsonDocument("Run", true);
            DBCursor cursor = cl.Explain(cond, null, null, null, 0, -1, 0, options);
            string actScanType = cursor.Next().GetValue("ScanType").ToString();
            cursor.Close();
            Assert.AreEqual(expScanType, actScanType, "wrong scan type. expect: " + expScanType + ", actual: " + actScanType);
        }
    }
}
