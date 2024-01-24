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
     * description: analyze cl
     * testcase:    14258
     * author:      linsuqiang
     * date:        2018/07/31
     */

    [TestClass]
    public class AnalyzeCl14258
    {
        private Sequoiadb sdb = null;
        private const string csName = "analyze14258";
        private const string analyzeClName = "analyze14258";
        private const string nonAnalyzeClName = "nonAnalyze14258";
        private DBCollection analyzeCl = null;
        private DBCollection nonAnalyzeCl = null;

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
            analyzeCl = cs.CreateCollection(analyzeClName);
            AnalyzeCommon.InsertDataWithIndex(analyzeCl);
            nonAnalyzeCl = cs.CreateCollection(nonAnalyzeClName);
            AnalyzeCommon.InsertDataWithIndex(nonAnalyzeCl);
        }

        [TestMethod]
        public void Test14258()
        {
            AnalyzeCommon.CheckScanTypeByExplain(analyzeCl, "ixscan");
            AnalyzeCommon.CheckScanTypeByExplain(nonAnalyzeCl, "ixscan");
            const string clFullName = csName + "." + analyzeClName;
            sdb.Analyze(new BsonDocument("Collection", clFullName));
            AnalyzeCommon.CheckScanTypeByExplain(analyzeCl, "ixscan");
            AnalyzeCommon.CheckScanTypeByExplain(nonAnalyzeCl, "ixscan");
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
    }
}
