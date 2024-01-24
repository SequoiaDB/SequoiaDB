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
     * description: createDomain (string domainName, BsonDocument options)；dropDomain (string domainName) ；getDomain (string domainName)；alter (BsonDocument options)
     *              1.createdomain no options and check  
     *              2.createdomain with options and check
     *              3.
     *              2. connect cata db and check cs attribute
     * testcase:    14557
     * author:      chensiqin
     * date:        2018/05/02
    */
    [TestClass]
    public class TestDomain14557
    {

        private Sequoiadb sdb = null;
        private Domain domain = null;
        private string domainName = "domain14557";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()] 
        public void Test14557()
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
            if (sdb.IsDomainExist(domainName))
            {
                sdb.DropDomain(domainName);
            }

            BsonDocument option = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            option.Add("Groups", arr);
            option.Add("AutoSplit", true);
            sdb.CreateDomain(domainName, option);
            //check domain name and other attributes
            BsonDocument expected = new BsonDocument();
            expected.Add("Name", domainName);
            expected.Add("Groups", arr);
            expected.Add("AutoSplit", true);
            CheckDomainInfo(expected);

            sdb.DropDomain(domainName);
            arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            arr.Add(dataGroupNames[2]);
            option = new BsonDocument();
            option.Add("Groups", arr);
            domain = sdb.CreateDomain(domainName, option);
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"AutoSplit", false}}}});
            arr = new BsonArray();
            arr.Add(dataGroupNames[2]);
            alterArray.Add(new BsonDocument{
                {"Name","add groups"},
                {"Args",new BsonDocument{{"Groups", arr}}}});
            option = new BsonDocument();
            option.Add("Alter", alterArray);
            domain.Alter(option);
            //check domain name and attribute
            expected = new BsonDocument();
            expected.Add("Name", domainName);
            expected.Add("Groups", arr);
            expected.Add("AutoSplit", false);
            CheckDomainInfo(expected);

            sdb.DropDomain(domainName);
            //check domain not exist
            CheckDomainInfo(null);

        }

        public void CheckDomainInfo(BsonDocument expected)
        {

            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", domainName);
            DBCursor cur = sdb.ListDomains(matcher, null, null, null);
            if (expected != null)
            {
                Assert.IsNotNull(cur.Next());
                BsonDocument obj = cur.Current();
                //check name group autosplit
                Console.WriteLine(expected.ToString());
                Assert.AreEqual(expected.GetElement("Name").Value, obj.GetElement("Name").Value);
                Assert.AreEqual(expected.GetElement("AutoSplit").Value, obj.GetElement("AutoSplit").Value);
                BsonArray arr = (BsonArray)obj.GetElement("Groups").Value;
                BsonArray arr2 = (BsonArray)expected.GetElement("Groups").Value;
                for (int i = 0; i < arr.Count; i++)
                {
                    BsonElement element = arr[i].ToBsonDocument().GetElement("GroupName");
                    arr2.Contains(element.Value);
                    arr2.Remove(element.Value);
                }
                Assert.AreEqual(0, arr2.Count());
            }
            else
            {
                Assert.IsNull(cur.Next());
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
