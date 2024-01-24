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
     * @Description seqDB-24874:驱动支持cs.GetDomainName()
     * @Author liuli
     * @Date 2021.12.22
     * @UpdatreAuthor liuli
     * @UpdateDate 2021.12.22
     * @version 1.00
    */
    [TestClass]
    public class GetDomainName24874
    {
        private Sequoiadb sdb = null;
        private string csName = "cs24874";
        private string clName = "cl24874";
        private string domainName1 = "domain24874_1";
        private string domainName2 = "domain24874_2";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test24874()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            if (sdb.IsCollectionSpaceExist(csName))
            {
                sdb.DropCollectionSpace(csName);
            }
            if (sdb.IsDomainExist(domainName1))
            {
                sdb.DropDomain(domainName1);
            }
            if (sdb.IsDomainExist(domainName2))
            {
                sdb.DropDomain(domainName2);
            }

            dataGroupNames = Common.getDataGroupNames(sdb);
            
            BsonDocument option = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            option.Add("Groups", arr);
            sdb.CreateDomain(domainName1, option);
            sdb.CreateDomain(domainName2, option);

            BsonDocument csOption = new BsonDocument();
            csOption.Add("Domain", domainName1);
            CollectionSpace dbcs = sdb.CreateCollectionSpace(csName, csOption);
            dbcs.CreateCollection(clName);

            String actDomainName = dbcs.GetDomainName();
            Assert.AreEqual(actDomainName, domainName1);

            BsonDocument setOption = new BsonDocument();
            setOption.Add("Domain", domainName2);
            dbcs.SetDomain(setOption);
            actDomainName = dbcs.GetDomainName();
            Assert.AreEqual(actDomainName, domainName2);

            dbcs.RemoveDomain();
            actDomainName = dbcs.GetDomainName();
            String expDomainName = "";
            Assert.AreEqual(actDomainName, expDomainName);
        }

        [TestCleanup()]
        public void TearDown()
        {
            sdb.DropCollectionSpace(csName);
            sdb.DropDomain(domainName1);
            sdb.DropDomain(domainName2);
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
