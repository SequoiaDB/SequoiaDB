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
     * description: analyze index
     * testcase:    14259
     * author:      linsuqiang
     * date:        2018/07/31
     */

    [TestClass]
    public class AnalyzeIndex14259
    {
        private Sequoiadb sdb = null;
        private const string csName = "analyze14259";
        private const string clName = "analyze14259";
        private DBCollection cl = null;
        private const int indexNum = 4;
        private string analyzeIdx = null;
        private List<string> nonAnalyzeIdxList = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            if (sdb.IsCollectionSpaceExist(csName))
            {
                sdb.DropCollectionSpace(csName);
            }
            CollectionSpace cs = sdb.CreateCollectionSpace(csName);
            cl = cs.CreateCollection(clName);
            InsertData(cl, indexNum);
            List<string> idxList = CreateIndexes(cl, indexNum);
            analyzeIdx = idxList.ElementAt(0);
            idxList.Remove(analyzeIdx);
            nonAnalyzeIdxList = idxList;
        }

        [TestMethod]
        public void Test14259()
        {
            CheckScanTypeByExplain(cl, analyzeIdx, "ixscan");
            for (int i = 0; i < nonAnalyzeIdxList.Count; ++i)
                CheckScanTypeByExplain(cl, nonAnalyzeIdxList.ElementAt(i), "ixscan");
            const string clFullName = csName + "." + clName;

            sdb.Analyze(new BsonDocument { { "Collection", clFullName }, { "Index", analyzeIdx } });
            CheckScanTypeByExplain(cl, analyzeIdx, "ixscan");
            for (int i = 0; i < nonAnalyzeIdxList.Count; ++i)
                CheckScanTypeByExplain(cl, nonAnalyzeIdxList.ElementAt(i), "ixscan");

            sdb.Analyze(new BsonDocument { { "Collection", clFullName }, { "Index", analyzeIdx }, { "Mode", 3 } });
            CheckScanTypeByExplain(cl, analyzeIdx, "ixscan");
            for (int i = 0; i < nonAnalyzeIdxList.Count; ++i)
                CheckScanTypeByExplain(cl, nonAnalyzeIdxList.ElementAt(i), "ixscan");
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

        private void InsertData(DBCollection cl, int fieldNum)
        {
            BsonDocument rec = new BsonDocument();
            for (int i = 0; i < fieldNum; ++i)
            {
                string fieldName = "field_" + i;
                rec.Add(fieldName, 0);
            }
            List<BsonDocument> recs = new List<BsonDocument>();
            int recNum = 2000;
            for (int i = 0; i < recNum; ++i)
            {
                recs.Add(new BsonDocument(rec));
            }
            cl.BulkInsert(recs, 0);
        }

        private List<string> CreateIndexes(DBCollection cl, int indexNum)
        {
            List<string> idxList = new List<string>();
            for (int i = 0; i < indexNum; ++i)
            {
                string fieldName = "field_" + i;
                string indexName = "idx_" + fieldName;
                cl.CreateIndex(indexName, new BsonDocument(fieldName, 1), false, false);
                idxList.Add(indexName);
            }
            return idxList;
        }

        private void CheckScanTypeByExplain(DBCollection cl, string indexName, string expScanType)
        {
            string fieldName = indexName.Substring(4, indexName.Length - 4); // pick up prefix "idx_"
            BsonDocument cond = new BsonDocument(fieldName, 0);
            BsonDocument options = new BsonDocument("Run", true);
            DBCursor cursor = cl.Explain(cond, null, null, null, 0, -1, 0, options);
            string actScanType = cursor.Next().GetValue("ScanType").ToString();
            cursor.Close();
            if (expScanType != actScanType)
            {
                throw new Exception("wrong scan type. expect: " + expScanType + ", actual: " + actScanType);
            }
        }
    }
}
