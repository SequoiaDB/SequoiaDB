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
     * description: create cs
     *              1.cs.Attributes() modify PageSize、LobPageSize、domian
     *              2. connect cata db and check cs attribute
     * testcase:    15238
     * author:      chensiqin
     * date:        2018/04/27
    */
    [TestClass]
    public class Meta15238
    {
        private Sequoiadb sdb = null;
        private CollectionSpace localcs = null;
        private string localCsName = "cs15238";
        private string domainName = "domain15238"; 
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test15238()
        {  
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
            localcs = sdb.CreateCollectionSpace(localCsName);
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            BsonDocument option = new BsonDocument();
            option.Add("Groups", arr);
            sdb.CreateDomain(domainName, option);
            option = new BsonDocument();
            option.Add("PageSize", 4096);
            option.Add("LobPageSize", 4096);
            option.Add("Domain", domainName);
            localcs.SetAttributes(option);
            CheckCSAttribute(option);
            sdb.DropCollectionSpace(localCsName);
            sdb.DropDomain(domainName);
        }

        private void CheckCSAttribute(BsonDocument expected)
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            DBCursor cur = null;
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
            Assert.AreEqual(expected.GetElement("PageSize").Value.ToString(), actual.GetElement("PageSize").Value.ToString());
            Assert.AreEqual(expected.GetElement("LobPageSize").Value.ToString(), actual.GetElement("LobPageSize").Value.ToString());
            Assert.AreEqual(expected.GetElement("Domain").Value.ToString(), actual.GetElement("Domain").Value.ToString());
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
