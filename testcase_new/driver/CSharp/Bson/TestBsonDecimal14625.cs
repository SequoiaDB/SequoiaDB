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
     *                BSONDecimal（Decimal value)
     *                1.插入一条decimal的数据，value参数覆盖：Decimal.MaxValue  Decimal.MinValue
     * testcase:     14625
     * author:       chensiqin
     * date:         2019/03/13
    */

    [TestClass]
    public class TestBsonDecimal14625
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14625";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14625()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            
            BsonDecimal decimal1 = new BsonDecimal(Decimal.MaxValue);
            BsonDecimal decimal2 = new BsonDecimal(Decimal.MinValue);
            
           
            BsonDocument record = new BsonDocument();
            record.Add("decimal1", decimal1);
            record.Add("decimal2", decimal2);
            cl.Insert(record);

            DBCursor cur = cl.Query();
            
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                Assert.AreEqual("{ \"$decimal\" : \"" + Decimal.MaxValue + "\" }", doc.GetElement("decimal1").Value.ToString());
                Assert.AreEqual("{ \"$decimal\" : \"" + Decimal.MinValue + "\" }", doc.GetElement("decimal2").Value.ToString());

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
