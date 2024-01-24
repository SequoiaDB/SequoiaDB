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
     * description: alter cappedcl attribute
     *              1.cs.enableCapped({Capped:true})
     *              2.cs.createCL("test",{Capped:true,Size:10000,Max:1000,AutoIndexId:false})
     *              3.check cl attibute info 
     *              4.disableCapped()
     *              5.check result
     *              6.delete cs.cl and cs.disableCapped()
     *              7. check result
     * testcase:    15232
     * author:      chensiqin
     * date:        2018/04/27
    */
    [TestClass]
    public class Meta15232
    {

        private Sequoiadb sdb = null;
        private CollectionSpace localcs = null;
        private DBCollection localcl = null;
        private string localCsName = "cs15232";
        private string clName = "cl15232";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test15232()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
            if (dataGroupNames.Count < 3)
            {
                return;
            }
            BsonDocument clop = new BsonDocument();
            clop.Add("Capped", true);
            localcs = sdb.CreateCollectionSpace(localCsName, clop);
            clop = new BsonDocument();
            clop.Add("Size", 1024);
            clop.Add("Max", 1000);
            clop.Add("AutoIndexId", false);
            clop.Add("Capped", true);
            localcl = localcs.CreateCollection(clName, clop);
            checkCLAttribute(true, clop);

            try
            {
                localcs.DisableCapped();
                Assert.Fail("expected thow BaseException but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-275, e.ErrorCode);
            }
            localcs.DropCollection(clName);
            localcs.DisableCapped();
            checkCLAttribute(false, clop);

            sdb.DropCollectionSpace(localCsName);
        }

        private void checkCLAttribute(bool capped, BsonDocument expected)
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            DBCursor cur = null;
            if (capped)
            {
                matcher.Add("Name", localCsName + "." + clName);
                cur = sdb.GetSnapshot(8, matcher, null, null);
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                long expectSize = 1024 * 1024 * 1024;//TODO:添加单位转换备注
                Assert.AreEqual(expectSize + "", actual.GetElement("Size").Value.ToString());
                Assert.AreEqual(expected.GetElement("Max").Value.ToString(), actual.GetElement("Max").Value.ToString());
                Assert.AreEqual("NoIDIndex | Capped", actual.GetElement("AttributeDesc").Value.ToString());
            }
            else
            {
                matcher.Add("Name", localCsName);
                ReplicaGroup cataRg = sdb.GetReplicaGroup("SYSCatalogGroup");
                Sequoiadb cataDB = cataRg.GetMaster().Connect();
                CollectionSpace sysCS = cataDB.GetCollectionSpace("SYSCAT");
                DBCollection sysCL = sysCS.GetCollection("SYSCOLLECTIONSPACES");
                DBQuery query = new DBQuery();
                query.Matcher = matcher;
                cur = sysCL.Query(query);
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                Assert.AreEqual(0 + "", actual.GetElement("Type").Value.ToString());
            }

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
