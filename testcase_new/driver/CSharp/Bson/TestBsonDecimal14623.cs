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
     *                1.插入一条decimal类型的数据，3个参数组合覆盖：
     *                  合法值，如n>=precision-scale、precision>scale、m>=scale、m 小于 scale 
     *                  非法值，如n小于precision-scale、precision小于或等于scale
     *                2.value参数实际的整数位数为n，value参数实际的小数位数为m
     * testcase:     14623
     * author:       chensiqin
     * date:         2019/03/13
    */

    [TestClass]
    public class TestBsonDecimal14623
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14623";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14623()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            //value参数实际的整数位数为n，value参数实际的小数位数为m
            BsonDecimal decimal1 = new BsonDecimal("123.456721", 9, 4);//n<=precision-scale precision>scale m>=scale
            BsonDecimal decimal2 = new BsonDecimal("123.456721", 9, 3);//m 小于 scale 
            BsonDecimal decimal3 = new BsonDecimal("123.456721", 9, 4);//m 等于 scale 
            try
            {
                BsonDecimal decimal4 = new BsonDecimal("123.456721", 9, 7);//n>precision-scale
                Assert.Fail("expected failed!");
            }
            catch (ArgumentException e)
            {
                 
            }
            try
            {
                BsonDecimal decimal5 = new BsonDecimal("123.456721", 6, 7);//n>precision<=scale
                Assert.Fail("expected failed!");
            }
            catch (ArgumentException e)
            {
                 
            }
            BsonDocument record = new BsonDocument();
            record.Add("decimal1", decimal1);
            record.Add("decimal2", decimal2);
            record.Add("decimal3", decimal3);
            cl.Insert(record);

            string expected = "{ \"decimal1\" : { \"$decimal\" : \"123.4567\", \"$precision\" : [9, 4] }, \"decimal2\" : { \"$decimal\" : \"123.457\", \"$precision\" : [9, 3] }, \"decimal3\" : { \"$decimal\" : \"123.4567\", \"$precision\" : [9, 4] } }";
            DBCursor cur = cl.Query();
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                Assert.AreEqual(record.ToString(), doc.ToString());
                doc.RemoveElement(doc.GetElement("_id"));
                Assert.AreEqual(expected, doc.ToString());
               
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
