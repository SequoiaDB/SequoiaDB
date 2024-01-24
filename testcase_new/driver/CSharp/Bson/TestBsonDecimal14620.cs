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
     *                BSONDecimal(String value, int precision, int scale)
     *                getValue()
     *                getPrecision()
     *                getScale()
     *                toBigDecimal ()
     *                1.插入一条decimal类型的数据，value参数覆盖：合法、非法：如合法值，正整数、负整数、0、正小数、负小数、小数缺少整数位，非法值：字母等
     *                2.获取decimal数据的value、precision、scale，检查是否正确
     * testcase:     14620
     * author:       chensiqin
     * date:         2019/03/12
    */

    [TestClass]
    public class TestBsonDecimal14620
    {

        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14620";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14620()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            //1.插入一条decimal类型的数据，value参数覆盖：合法、非法：如合法值，正整数、负整数、0、正小数、负小数、小数缺少整数位，非法值：字母等
            BsonDecimal decimal1 = new BsonDecimal("123456", 8, 2);
            BsonDecimal decimal2 = new BsonDecimal("-123456", 8, 2);
            BsonDecimal decimal3 = new BsonDecimal("0", 2, 2);
            BsonDecimal decimal4 = new BsonDecimal("1.1111652", 9, 0);
            BsonDecimal decimal5 = new BsonDecimal("-1.1111652", 10, 1);
            BsonDecimal decimal6 = new BsonDecimal("0.1111652", 11, 5);
            
            BsonDocument record = new BsonDocument();
            record.Add("decimal1",  decimal1);
            record.Add("decimal2", decimal2);
            record.Add("decimal3", decimal3);
            record.Add("decimal4", decimal4);
            record.Add("decimal5", decimal5);
            record.Add("decimal6", decimal6);
            cl.Insert(record);

            try
            {
                BsonDecimal decimal7 = new BsonDecimal("sdb", 5, 2);
                Assert.Fail("expected failed!");
            }
            catch (ArgumentException e)
            {
                Console.WriteLine(e);
            }

            DBCursor cur = cl.Query();
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                Assert.AreEqual(record.ToString(), doc.ToString());
            }
            cur.Close();
            Assert.AreEqual(1, count);

            //2.获取decimal数据的value、precision、scale，检查是否正确
            Assert.AreEqual("123456.00", decimal1.Value);
            Assert.AreEqual("-123456.00", decimal2.Value);
            Assert.AreEqual("0.00", decimal3.Value);
            Assert.AreEqual("1", decimal4.Value);
            Assert.AreEqual("-1.1", decimal5.Value);
            Assert.AreEqual("0.11117", decimal6.Value);

            Assert.AreEqual(8, decimal1.Precision);
            Assert.AreEqual(8, decimal2.Precision);
            Assert.AreEqual(2, decimal3.Precision);
            Assert.AreEqual(9, decimal4.Precision);
            Assert.AreEqual(10, decimal5.Precision);
            Assert.AreEqual(11, decimal6.Precision);

            Assert.AreEqual(2, decimal1.Scale);
            Assert.AreEqual(2, decimal2.Scale);
            Assert.AreEqual(2, decimal3.Scale);
            Assert.AreEqual(0, decimal4.Scale);
            Assert.AreEqual(1, decimal5.Scale);
            Assert.AreEqual(5, decimal6.Scale);
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
