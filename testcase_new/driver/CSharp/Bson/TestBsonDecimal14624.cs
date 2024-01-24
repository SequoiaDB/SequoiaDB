using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Bson
{
    /**
     * description:  
     *                BSONDecimal(String value)
     *                1.插入一条decimal的数据，value参数覆盖：合法值，如典型值m+n小于147455、边界值m+n=147455,非法值，如m+n=147456
     * testcase:     14624
     * author:       chensiqin
     * date:         2019/03/13
    */

    [TestClass]
    public class TestBsonDecimal14624
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14624";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14624() 
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            //插入一条decimal的数据，value参数覆盖：合法值，如典型值m+n小于147455、边界值m+n=147455,非法值，如m+n=147456
            //n最大131072  m最大值16383 
            BsonDecimal decimal1 = new BsonDecimal("123.456");
            string n = "";
            for (int i = 1; i <= 131072; i++)
            {
                n += "1";
            }
            string m = "";
            for (int i = 1; i <= 16383; i++)
            {
                m += "1";
            }
            BsonDecimal decimal2 = new BsonDecimal(n + "."+ m);
            try
            {
                BsonDecimal decimal3 = new BsonDecimal(n + ".1" + m);
            }
            catch (ArgumentException e)
            {
                Console.WriteLine(e);
            }
            BsonDocument record = new BsonDocument();
            record.Add("decimal1", decimal1);
            record.Add("decimal2", decimal2);
            cl.Insert(record);

            DBCursor cur = cl.Query();
            string expected1 = "123.456";
            string expected2 = n + "." + m;
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                Assert.AreEqual("{ \"$decimal\" : \""+expected1+"\" }", doc.GetElement("decimal1").Value.ToString());
                Assert.AreEqual("{ \"$decimal\" : \"" + expected2 + "\" }", doc.GetElement("decimal2").Value.ToString());

            }
            cur.Close();
            Assert.AreEqual(1, count);
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
