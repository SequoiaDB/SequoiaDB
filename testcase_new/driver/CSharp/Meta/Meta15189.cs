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
     * description: createDomain1(group1,group2,group3),createDomain(group1,group2)
     *              cs setDomain(domain1),setDomain(domain2),removeDomain(domain2),connect cata node check result in every step
     * testcase:    15189
     * author:      chensiqin
     * date:        2018/04/25
    */
    [TestClass]
    public class Meta15189
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl15189";
        private string localCsName = "cs15189";
        private string domainName1 = "domain15189_1";
        private string domainName2 = "domain15189_2";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            
        }

        [TestMethod()]
        public void TestDelete15189()
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
            //create domain and cl
            createDomians();
            //setdomain1
            BsonDocument option = new BsonDocument();
            option.Add("Domain", domainName1);
            cs.SetDomain(option);
            checkCSCataInfo(domainName1);

            //setdomain2
            option = new BsonDocument();
            option.Add("Domain", domainName2);
            cs.SetDomain(option);
            checkCSCataInfo(domainName2);

            //remove domain
            cs.RemoveDomain();
            checkCSCataInfo("");

            sdb.DropCollectionSpace(localCsName);
            sdb.DropDomain(domainName1);
            sdb.DropDomain(domainName2);
        }

        public void createDomians()
        {
            //create domain1
            BsonDocument option = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            arr.Add(dataGroupNames[2]);
            option.Add("Groups", arr);
            Domain domain1 = sdb.CreateDomain(domainName1, option);

            //create domian2
            option = new BsonDocument();
            arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            option.Add("Groups", arr);
            Domain domain2 = sdb.CreateDomain(domainName2, option);

            //createcl
            cs = sdb.CreateCollectionSpace(localCsName);
            option = new BsonDocument();
            option.Add("Group", dataGroupNames[0]);
            cl = cs.CreateCollection(clName, option);

        }

        public void checkCSCataInfo(string expectDomain)
        {
            DBQuery query = new DBQuery();
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", localCsName);
            ReplicaGroup cataRg =  sdb.GetReplicaGroup("SYSCatalogGroup");
            Sequoiadb cataDB = cataRg.GetMaster().Connect();
            CollectionSpace sysCS= cataDB.GetCollectionSpace("SYSCAT");
            DBCollection sysCL = sysCS.GetCollection("SYSCOLLECTIONSPACES");
            query.Matcher = matcher;
            DBCursor cur = sysCL.Query(query);
            Assert.IsNotNull(cur.Next());
            BsonDocument re = cur.Current();
            if (expectDomain != "")
            {
                Assert.AreEqual(expectDomain, re.GetElement("Domain").Value.ToString());
            }
            else
            {
                try
                {
                    re.GetElement("Domain");
                }
                catch (Exception e)
                {
                    Assert.AreEqual("Element 'Domain' not found.", e.Message);
                }
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
