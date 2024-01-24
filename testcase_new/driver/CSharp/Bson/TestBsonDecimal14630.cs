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
     *                  ToDecimal ()
     *                  ToDouble ()
     *                  ToInt64 ()
     *                  ToInt32 ()
     *                  1.构建BsonDecimal数据，将不同数据分别转换为Deciaml、Double、Int64、int32类型，检查转后bson的内容是否正确
     *                  2.将转换数据插入到sdb中，查询数据类型
     * testcase:     seqDB-14630
     * author:       chensiqin
     * date:         2019/03/14
    */

    [TestClass]
    public class TestBsonDecimal14630
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14630";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14630()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            BsonDecimal bsonDecimal = new BsonDecimal("123.456789");
            Decimal decimal1 =  bsonDecimal.ToDecimal();
            double d = bsonDecimal.ToDouble();
            int i32 = bsonDecimal.ToInt32();
            long i64 = bsonDecimal.ToInt64();
            BsonDocument record = new BsonDocument();
            record.Add("decimalType", decimal1);
            record.Add("doubleType", d);
            record.Add("intType", i32);
            record.Add("longType", i64);
            cl.Insert(record);

            //检查转后的数据类型
            Assert.AreEqual("123.456789", decimal1.ToString());
            Assert.AreEqual(123.456789, d);
            Assert.AreEqual(123, i32);
            Assert.AreEqual(123, i64);

            string expected = "{ \"decimalType\" : { \"$decimal\" : \"123.456789\" }, \"doubleType\" : 123.456789, \"intType\" : 123, \"longType\" : 123 }";
            DBCursor cur = cl.Query();
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                doc.Remove("_id");
                Assert.AreEqual(expected, doc.ToString());
            }
            cur.Close();
            Assert.AreEqual(1, count);

            expected = "{ \"decimalType\" : \"decimal\", \"doubleType\" : \"double\", \"intType\" : \"int32\", \"longType\" : \"int64\" }";
            DBQuery query = new DBQuery();
            BsonDocument selector = new BsonDocument();
            selector.Add("decimalType", new BsonDocument("$type", 2));
            selector.Add("doubleType", new BsonDocument("$type", 2));
            selector.Add("intType", new BsonDocument("$type", 2));
            selector.Add("longType", new BsonDocument("$type", 2));
            query.Selector = selector;
            cur = cl.Query(query);
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                doc.Remove("_id");
                Assert.AreEqual(expected, doc.ToString());
            }
            cur.Close();
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
