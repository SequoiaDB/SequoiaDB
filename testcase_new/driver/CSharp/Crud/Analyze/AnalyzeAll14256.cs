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
     * description: analyze globally
     * testcase:    14256
     * author:      linsuqiang
     * date:        2018/07/31
     */

    [TestClass]
    public class AnalyzeAll14256
    {

        private Sequoiadb sdb = null;
        private const string csBaseName = "analyze14256";
        private const int csNum = 2;
        private const int clNumPerCs = 2;
        List<DBCollection> clList = new List<DBCollection>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            for (int i = 0; i < csNum; ++i)
            {
                string csName = csBaseName + "_" + i;
                if (sdb.IsCollectionSpaceExist(csName))
                {
                    sdb.DropCollectionSpace(csName);
                }
                CollectionSpace cs = sdb.CreateCollectionSpace(csName);
                for (int j = 0; j < clNumPerCs; ++j)
                {
                    string clName = csName + "_" + j;
                    DBCollection cl = cs.CreateCollection(clName);
                    AnalyzeCommon.InsertDataWithIndex(cl);
                    clList.Add(cl);
                }
            }
        }

        [TestMethod]
        public void Test14256()
        {
            
            for (int i = 0; i < clList.Count; ++i)
            {
                AnalyzeCommon.CheckScanTypeByExplain(clList.ElementAt(i), "ixscan");
            }
            sdb.Analyze();
            for (int i = 0; i < clList.Count; ++i)
            {
                AnalyzeCommon.CheckScanTypeByExplain(clList.ElementAt(i), "ixscan");
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            for (int i = 0; i < csNum; ++i)
            {
                string csName = csBaseName + "_" + i;
                if (sdb.IsCollectionSpaceExist(csName))
                {
                    sdb.DropCollectionSpace(csName);
                }
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
