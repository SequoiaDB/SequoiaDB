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
     * description: alter domain attribute
     *              1.create domain
     *              2.addGroups and check domain attribute
     *              3.setGroups and check domain attribute
     *              4.removeGroups and check domain attribute
     * testcase:    15186
     * author:      chensiqin
     * date:        2018/04/27
    */
    [TestClass]
    public class Meta15186
    {
        private Sequoiadb sdb = null;
        private Domain domain = null;
        private string domainName = "domain15186";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test15186()
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
            BsonArray arr = new BsonArray();
            BsonDocument option = new BsonDocument();
            arr.Add(dataGroupNames[0]);
            option.Add("Groups", arr);
            domain = sdb.CreateDomain(domainName, option);

            //1.addGroups and check domain attribute
            arr = new BsonArray();
            option = new BsonDocument();
            arr.Add(dataGroupNames[1]);
            arr.Add(dataGroupNames[2]);
            option.Add("Groups", arr);
            domain.AddGroups(option);
            BsonArray expectedArr = new BsonArray();
            expectedArr.Add(dataGroupNames[0]);
            expectedArr.Add(dataGroupNames[1]);
            expectedArr.Add(dataGroupNames[2]);
            CheckDomainInfo(expectedArr);

            //2.setGroups and check domain attribute
            arr = new BsonArray();
            option = new BsonDocument();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            option.Add("Groups", arr);
            expectedArr = new BsonArray();
            expectedArr.Add(dataGroupNames[0]);
            expectedArr.Add(dataGroupNames[1]);
            domain.SetGroups(option);
            CheckDomainInfo(expectedArr);

            //3.removeGroups and check domain attribute
            arr = new BsonArray();
            option = new BsonDocument();
            arr.Add(dataGroupNames[0]);
            option.Add("Groups", arr);
            domain.RemoveGroups(option);
            expectedArr = new BsonArray();
            expectedArr.Add(dataGroupNames[1]);
            CheckDomainInfo(expectedArr);

            sdb.DropDomain(domainName);
        }

        public void CheckDomainInfo(BsonArray expected)
        {

            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", domainName);
            DBCursor cur = sdb.ListDomains(matcher, null, null, null);
            Assert.IsNotNull(cur.Next());
            BsonDocument obj = cur.Current();
            BsonArray arr = (BsonArray)obj.GetElement("Groups").Value;
            for (int i = 0; i < arr.Count; i++)
            {
                BsonElement element = arr[i].ToBsonDocument().GetElement("GroupName");
                expected.Contains(element.Value);
                expected.Remove(element.Value);
            }
            Assert.AreEqual(0, expected.Count());
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
