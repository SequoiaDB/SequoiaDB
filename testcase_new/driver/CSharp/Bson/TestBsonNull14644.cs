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
     *                BsonNull (int value)         CompareTo (BsonNull other)
     *                CompareTo (BsonValue other)   Equals (BsonNull rhs)
     *                Equals (object obj)           GetHashCode ()
     *                ToString ()
     *                1.分别测试BsonNull类中所有接口,构造BsonNull类型数据（覆盖边界值），插入数据到sdb中
     *                2.使用Compare接口比较两个BsonNull类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                3.使用equals接口比较两个BsonNull类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                4、获取数据的hashCode值
     *                5、执行toString()方法，检查结果
     * testcase:    14644
     * author:      chensiqin
     * date:        2019/03/05
    */

    [TestClass]
    public class TestBsonNull14644
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14644";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14644()
        {

            prepareData();
            checkData();

            BsonDocument record = new BsonDocument();
            record.Add("value", BsonNull.Value);
            Assert.AreEqual("{ \"value\" : null }", record.ToString());
            
            //Compare接口比较两个BsonNull类型数据（覆盖相同和不同值比较、不同类型相同值比较
            //equals接口比较两个BsonNull类型数据（覆盖相同和不同值比较、不同类型相同值比较）

            //相同值比较
            BsonNull bsonNull1 = BsonNull.Value;
            BsonNull bsonNull2 = BsonNull.Value;
            Assert.AreEqual(0, bsonNull1.CompareTo(bsonNull2));
            Assert.AreEqual(true, bsonNull1.Equals(bsonNull2));

            //获取数据的hashCode值,相同对象hashcode一致，不同对象相同值hashcode值一致
            Assert.AreEqual(bsonNull1.GetHashCode(), bsonNull1.GetHashCode());
            Assert.AreEqual(bsonNull1.GetHashCode(), bsonNull2.GetHashCode());

            
            //不同类型相同值比较
            record = new BsonDocument();
            record.Add("value",null);
            Assert.AreEqual(1, bsonNull1.CompareTo(null));
            Assert.AreEqual(false, bsonNull1.Equals(null));

            //执行tostring
            Assert.AreEqual("BsonNull", bsonNull1.ToString());
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
            record.Add("value", BsonNull.Value);
            cl.Insert(record);
        }

        private void checkData()
        {
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(0, doc.GetElement("value").Value.CompareTo(BsonNull.Value));
            }
            cur.Close();
        }
    }
}
