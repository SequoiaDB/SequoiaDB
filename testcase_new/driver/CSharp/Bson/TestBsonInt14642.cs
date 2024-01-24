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
     *                BsonInt32 (int value)         CompareTo (BsonInt32 other)
     *                CompareTo (BsonValue other)   Equals (BsonInt3 rhs)
     *                Equals (object obj)           GetHashCode ()
     *                ToString ()
     *                1.分别测试BsonInt类中所有接口,构造BsonInt类型数据（覆盖边界值），插入数据到sdb中
     *                2.使用Compare接口比较两个BsonInt类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                3.使用equals接口比较两个BsonInt类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                4、获取数据的hashCode值
     *                5、执行toString()方法，检查结果
     * testcase:    14642
     * author:      chensiqin
     * date:        2019/03/04
    */

    [TestClass]
    public class TestBsonInt14642
    {

        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14642";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14642()
        {
            
            prepareData();
            checkData();

            //Compare接口比较两个BsonInt类型数据（覆盖相同和不同值比较、不同类型相同值比较
            //equals接口比较两个BsonInt类型数据（覆盖相同和不同值比较、不同类型相同值比较）

            //相同类型相同值
            BsonInt32 bsonInt1 = new BsonInt32(131);
            BsonInt32 bsonInt2 = new BsonInt32(131);
            Assert.AreEqual(0, bsonInt1.CompareTo(bsonInt2));
            Assert.AreEqual(true, bsonInt1.Equals(bsonInt2));

            //获取数据的hashCode值,相同对象hashcode一致，不同对象相同值hashcode值一致
            Assert.AreEqual(bsonInt1.GetHashCode(), bsonInt1.GetHashCode());
            Assert.AreEqual(bsonInt1.GetHashCode(), bsonInt2.GetHashCode());
            
            //相同类型不同值
            bsonInt1 = new BsonInt32(1314);
            bsonInt2 = new BsonInt32(131);
            Assert.AreEqual(1, bsonInt1.CompareTo(bsonInt2));
            Assert.AreEqual(false, bsonInt1.Equals(bsonInt2));
            //获取数据的hashCode值,不同对象不同值hashcode不一致
            Assert.AreNotEqual(bsonInt1.GetHashCode(), bsonInt2.GetHashCode());

            //不同类型相同值
            Assert.AreEqual(0, bsonInt1.CompareTo(new BsonDouble(1314)));

            //Assert.AreEqual(true, bsonInt1.Equals(new BsonDouble(1314)));//TODO:SEQUOIADBMAINSTREAM-4280
            //Assert.AreEqual(true, bsonInt1.Equals(new BsonInt64(1314)));//TODO:SEQUOIADBMAINSTREAM-4280

            //执行tostring
            Assert.AreEqual("1314", bsonInt1.ToString());
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

        private void prepareData()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            BsonDocument record = new BsonDocument();
            record.Add("IntMaxValue", new BsonInt32(int.MaxValue) );
            record.Add("IntMinValue", new BsonInt32(int.MinValue) );
            record.Add("IntValue",    new BsonInt32(14642) );
            cl.Insert(record);
        }

        private void checkData()
        {
            DBCursor cur = cl.Query();
            while (cur.Next() != null) 
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(int.MaxValue+"", doc.GetElement("IntMaxValue").Value.ToString());
                Assert.AreEqual(int.MinValue + "", doc.GetElement("IntMinValue").Value.ToString());
                Assert.AreEqual(14642 + "", doc.GetElement("IntValue").Value.ToString());
            }
            cur.Close();
        }
    }
}
