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
     *                1.插入一条decimal类型的数据，precision参数覆盖：合法、非法：如合法值1、1000，非法值：0、1001、a、-1、1.2 
     * testcase:     14621
     * author:       chensiqin
     * date:         2019/03/12
    */

    [TestClass]
    public class TestBsonDecimal14621
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14621";


        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14621()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            BsonDecimal decimal1 = new BsonDecimal("1", 1, 0);
            BsonDecimal decimal2 = new BsonDecimal("1", 100, 1);
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
                Assert.AreEqual(record.ToString(), doc.ToString());
            }
            cur.Close();
            Assert.AreEqual(1, count);

            try
            {
                BsonDecimal decimal3 = new BsonDecimal("1", 0, 1);
                Assert.Fail("expected failed!");
            }
            catch (ArgumentException e)
            {
                
            }
            try
            {
                BsonDecimal decimal4 = new BsonDecimal("1", 1001, 1);
                Assert.Fail("expected failed!");
            }
            catch (ArgumentException e)
            {
                
            }
            try
            {
                BsonDecimal decimal5 = new BsonDecimal("1", -1, 1);
                Assert.Fail("expected failed!");
            }
            catch (ArgumentException e)
            {
                
            }
            
            
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
