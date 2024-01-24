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
     *              2.domain.Alter() domain.alter({Alter:[ {Name:"set attributes", Args:{AutoSplit:true}}, {Name:"set attributes", Args: {Name:‘test’}}, {Name:"add groups", Args: {Groups:['group1']}}], Options:{IgnoreException:true}} )
     * testcase:    15225
     * author:      chensiqin
     * date:        2018/04/27
    */
    [TestClass]
    public class Meta15225
    {
        private Sequoiadb sdb = null;
        private Domain domain = null;
        private string domainName = "domain15225";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test15225()
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
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            BsonDocument option = new BsonDocument();
            option.Add("Groups", arr);
            domain = sdb.CreateDomain(domainName, option);
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"Name", "names11"}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"AutoSplit", true}}}});
            arr = new BsonArray();
            arr.Add(dataGroupNames[2]);
            alterArray.Add(new BsonDocument{
                {"Name","add groups"},
                {"Args",new BsonDocument{{"Groups", arr}}}});

            option = new BsonDocument();
            option.Add("Alter", alterArray);
            option.Add("Options", new BsonDocument { { "IgnoreException", true } });
            domain.Alter(option);
            CheckDomainInfo();
            sdb.DropDomain(domainName);
        }

        public void CheckDomainInfo()
        {

            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", domainName);
            DBCursor cur = sdb.ListDomains(matcher, null, null, null);
            Assert.IsNotNull(cur.Next());
            BsonDocument obj = cur.Current();
            //check name group autosplit
            Assert.AreEqual(domainName, obj.GetElement("Name").Value);
            Assert.AreEqual(true, obj.GetElement("AutoSplit").Value);
            BsonArray arr = (BsonArray)obj.GetElement("Groups").Value;
            for (int i = 0; i < arr.Count; i++)
            {
                BsonElement element = arr[i].ToBsonDocument().GetElement("GroupName");
                dataGroupNames.Contains(element.Value.ToString());
                dataGroupNames.Remove(element.Value.ToString());
            }
            Assert.AreEqual(0, dataGroupNames.Count());
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
