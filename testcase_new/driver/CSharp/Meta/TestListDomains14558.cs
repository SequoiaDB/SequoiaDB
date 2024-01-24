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
     * description: listDomains (BsonDocument matcher, BsonDocument selector, BsonDocument orderBy, BsonDocument hint)；Name ()；SequoiaDB ()；listCS ()；listCL ()
     *              1.create many cs and domain and create ccs.cl 
     *              2.listdomains and check domain info 
     *              3.获取其中一个domain下所有CS，返回CS信息正确； 
     *              4.检查该domain下所有CL，返回Cl信息正确；
     *              5.检查当前domain连接的sdb，返回当前连接的sdb信息正确； 
     *              6.切换使用不同domain，获取当前domainName，返回信息正确
     * testcase:    14558
     * author:      chensiqin
     * date:        2018/05/02
    */
    [TestClass]
    public class TestListDomains14558
    {
        private Sequoiadb sdb = null;
        private string domainName1 = "domain14558_1";
        private string domainName2 = "domain14558_2";
        private string domainName3 = "domain14558_3";
        private string csName1 = "cs14558_1";
        private string csName2 = "cs14558_2";
        private string csName3 = "cs14558_3";
        private string clName1 = "cl14558_1";
        private string clName2 = "cl14558_2";
        private string clName3 = "cl14558_3";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test14558()
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
            CreateDomians();
            CreateCS();
            CreateCL();
            CheckListDomainInfo();
            CheckListCS();
            CheckListCL();
            CheckDomainSequoiaDB();

            sdb.DropCollectionSpace(csName1);
            sdb.DropCollectionSpace(csName2);
            sdb.DropCollectionSpace(csName3);
            sdb.DropDomain(domainName1);
            sdb.DropDomain(domainName2);
            sdb.DropDomain(domainName3);
        }

        private void CreateCL()
        {
            CollectionSpace cs = sdb.GetCollectionSpace(csName1);
            cs.CreateCollection(clName1);
            cs.CreateCollection(clName2);
            cs.CreateCollection(clName3);

            cs = sdb.GetCollectionSpace(csName2);
            cs.CreateCollection(clName1);
            cs.CreateCollection(clName2);
            cs.CreateCollection(clName3);

            cs = sdb.GetCollectionSpace(csName3);
            cs.CreateCollection(clName1);
            cs.CreateCollection(clName2);
            cs.CreateCollection(clName3);

        }

        private void CreateCS()
        {
            //createCS1
            BsonDocument option = new BsonDocument();
            option.Add("Domain", domainName1);
            sdb.CreateCollectionSpace(csName1, option);

            //createCS2
            option = new BsonDocument();
            option.Add("Domain", domainName2);
            sdb.CreateCollectionSpace(csName2, option);

            //createCS3
            option = new BsonDocument();
            option.Add("Domain", domainName3);
            sdb.CreateCollectionSpace(csName3, option);
        }

        private void CreateDomians()
        {
            //create domain1
            BsonDocument option = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            option.Add("Groups", arr);
            sdb.CreateDomain(domainName1, option);

            //create domian2
            option = new BsonDocument();
            arr = new BsonArray();
            arr.Add(dataGroupNames[1]);
            arr.Add(dataGroupNames[2]);
            option.Add("Groups", arr);
            sdb.CreateDomain(domainName2, option);

            //create domian2
            option = new BsonDocument();
            arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[2]);
            option.Add("Groups", arr);
            sdb.CreateDomain(domainName3, option);

        }

        private void CheckListDomainInfo()
        {
            BsonDocument selector = new BsonDocument();
            selector.Add("Name", 1);
            DBCursor cur = sdb.ListDomains(null, selector, null, null);
            List<string> actual = new List<string>();
            while(cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            Assert.IsTrue(actual.Contains(domainName1));
            Assert.IsTrue(actual.Contains(domainName2));
            Assert.IsTrue(actual.Contains(domainName3));
        }

        private void CheckListCS()
        {
            Domain domain = sdb.GetDomain(domainName1);
            List<string> actual = new List<string>();
            DBCursor cur = domain.ListCS();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            actual.Remove(csName1);
            Assert.AreEqual(0, actual.Count());

            domain = sdb.GetDomain(domainName2);
            actual = new List<string>();
            cur = domain.ListCS();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            actual.Remove(csName2);
            Assert.AreEqual(0, actual.Count());

            domain = sdb.GetDomain(domainName3);
            actual = new List<string>();
            cur = domain.ListCS();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            actual.Remove(csName3);
            Assert.AreEqual(0, actual.Count());
        }

        private void CheckListCL()
        {
            Domain domain = sdb.GetDomain(domainName1);
            List<string> actual = new List<string>();
            DBCursor cur = domain.ListCL();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            actual.Remove(csName1+"."+clName1);
            actual.Remove(csName1 + "." + clName2);
            actual.Remove(csName1 + "." + clName3);
            Assert.AreEqual(0, actual.Count());

            domain = sdb.GetDomain(domainName2);
            actual = new List<string>();
            cur = domain.ListCL();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            actual.Remove(csName2 + "." + clName1);
            actual.Remove(csName2 + "." + clName2);
            actual.Remove(csName2 + "." + clName3);
            Assert.AreEqual(0, actual.Count());

            domain = sdb.GetDomain(domainName3);
            actual = new List<string>();
            cur = domain.ListCL();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            actual.Remove(csName3 + "." + clName1);
            actual.Remove(csName3 + "." + clName2);
            actual.Remove(csName3 + "." + clName3);
            Assert.AreEqual(0, actual.Count());
        }

        private void CheckDomainSequoiaDB()
        {
            /*
             4、检查当前domain连接的sdb，返回当前连接的sdb信息正确；
             5、切换使用不同domain，获取当前domainName，返回信息正确
            */
            Domain domain = sdb.GetDomain(domainName1);
            ServerAddress doc = domain.SequoiaDB.ServerAddr;
            Console.WriteLine(doc.ToString());

            Domain domain1 = sdb.GetDomain(domainName1);
            Domain domain2 = sdb.GetDomain(domainName1);
            Domain domain3 = sdb.GetDomain(domainName1);
            Assert.AreEqual(domainName1, domain1.Name);
            Assert.AreEqual(domainName1, domain2.Name);
            Assert.AreEqual(domainName1, domain3.Name);
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
