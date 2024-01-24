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
     * description: createDomain
     *              createdomain and alter domain and check
     * testcase:    15187
     * author:      chensiqin
     * date:        2018/04/25
    */
    [TestClass]
    public class Meta15187
    {
        private Sequoiadb sdb = null;
        private string domainName1 = "domain15187_1"; 
        private string domainName2 = "domain15187_2";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }
         	
        [TestMethod()]
        public void TestDelete15187() 
        {
            List<string> dataGroupNames = Common.getDataGroupNames(sdb);
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
            if (dataGroupNames.Count < 3)
            {
                return;
            }
            BsonDocument option = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            option.Add("Groups", arr);
            Domain domain = sdb.CreateDomain(domainName1, option);

            //alter  domain attribute
            option = new BsonDocument();
            arr = new BsonArray();
            arr.Add(dataGroupNames[1]);
            arr.Add(dataGroupNames[2]);
            option.Add("Groups", arr);
            option.Add("Name", domainName2);
            option.Add("AutoSplit", true);
            try
            {
                domain.SetAttributes(option);
                Assert.Fail("expected thow BaseException but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            
            option = new BsonDocument();
            arr = new BsonArray();
            arr.Add(dataGroupNames[1]);
            arr.Add(dataGroupNames[2]);
            option.Add("Groups", arr);
            option.Add("AutoSplit", true);
            domain.SetAttributes(option);
            CheckDomainInfo(domainName1, dataGroupNames[1], dataGroupNames[2]);

            sdb.DropDomain(domainName1);
        }

        public void CheckDomainInfo(String name, string groupnam1, string groupname2)
        {
     
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", name);
            DBCursor cur = sdb.ListDomains(matcher, null, null, null);
            Assert.IsNotNull(cur.Next());
            BsonDocument obj = cur.Current();
            //check name group autosplit
            Assert.AreEqual(name, obj.GetElement("Name").Value);
            Assert.AreEqual(true, obj.GetElement("AutoSplit").Value);
            BsonArray arr = (BsonArray)obj.GetElement("Groups").Value;
            for (int i = 0; i < arr.Count; i++)
            {
                BsonElement element = arr[i].ToBsonDocument().GetElement("GroupName");
                if (!groupnam1.Equals(element.Value.ToString()) && !groupname2.Equals(element.Value.ToString()))
                {
                    Assert.Fail("alter domain Groups result is error!");
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
