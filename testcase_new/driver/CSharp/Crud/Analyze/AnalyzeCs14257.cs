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
     * description: analyze cs
     * testcase:    14257
     * author:      linsuqiang
     * date:        2018/07/31
     */

    [TestClass]
    public class AnalyzeCs14257
    {
        private Sequoiadb sdb = null;
        private const string analyzeCsName = "analyzeCs14257";
        private const string nonAnalyzeCsName = "nonAnalyzeCs14257";
        private const int clNumPerCs = 2;
        List<DBCollection> analyzeClList = new List<DBCollection>();
        List<DBCollection> nonAnalyzeClList = new List<DBCollection>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            if (sdb.IsCollectionSpaceExist(analyzeCsName))
            {
                sdb.DropCollectionSpace(analyzeCsName);
            }
            if (sdb.IsCollectionSpaceExist(nonAnalyzeCsName))
            {
                sdb.DropCollectionSpace(nonAnalyzeCsName);
            }
            CollectionSpace analyzeCs = sdb.CreateCollectionSpace(analyzeCsName);
            for (int i = 0; i < clNumPerCs; ++i)
            {
                string clName = analyzeCsName + "_" + i;
                DBCollection cl = analyzeCs.CreateCollection(clName);
                AnalyzeCommon.InsertDataWithIndex(cl);
                analyzeClList.Add(cl);
            }
            CollectionSpace nonAnalyzeCs = sdb.CreateCollectionSpace(nonAnalyzeCsName);
            for (int i = 0; i < clNumPerCs; ++i)
            {
                string clName = nonAnalyzeCsName + "_" + i;
                DBCollection cl = nonAnalyzeCs.CreateCollection(clName);
                AnalyzeCommon.InsertDataWithIndex(cl);
                nonAnalyzeClList.Add(cl);
            }
        }

        [TestMethod]
        public void Test14257()
        {
            for (int i = 0; i < analyzeClList.Count; ++i)
                AnalyzeCommon.CheckScanTypeByExplain(analyzeClList.ElementAt(i), "ixscan");
            for (int i = 0; i < nonAnalyzeClList.Count; ++i)
                AnalyzeCommon.CheckScanTypeByExplain(nonAnalyzeClList.ElementAt(i), "ixscan");
            
            sdb.Analyze(new BsonDocument("CollectionSpace", analyzeCsName).Add("Mode", 2));
            for (int i = 0; i < analyzeClList.Count; ++i)
                AnalyzeCommon.CheckScanTypeByExplain(analyzeClList.ElementAt(i), "ixscan");
            for (int i = 0; i < nonAnalyzeClList.Count; ++i)
                AnalyzeCommon.CheckScanTypeByExplain(nonAnalyzeClList.ElementAt(i), "ixscan");
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb.IsCollectionSpaceExist(analyzeCsName))
                sdb.DropCollectionSpace(analyzeCsName);
            if (sdb.IsCollectionSpaceExist(nonAnalyzeCsName))
                sdb.DropCollectionSpace(nonAnalyzeCsName);
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
