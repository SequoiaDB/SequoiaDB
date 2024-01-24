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
     *                   BsonArray ()     
     *                   BsonArray (IEnumerable< bool > values)
     *                   BsonArray (IEnumerable< BsonValue > values)
     *                   BsonArray (IEnumerable< DateTime > values)
     *                   BsonArray (IEnumerable< double > values)
     *                   BsonArray (IEnumerable< int > values)
     *                   BsonArray (IEnumerable< long > values)
     *                   BsonArray (IEnumerable< ObjectId > values)
     *                   BsonArray (IEnumerable< string > values)
     *                   BsonArray (IEnumerable values)
     *                   BsonArray (int capacity)
     *                   Add (BsonValue value)
     *                   AddRange (IEnumerable< bool > values)
     *                   AddRange (IEnumerable< BsonValue > values)
     *                   AddRange (IEnumerable< DateTime > values)
     *                   AddRange (IEnumerable< double > values)
     *                   AddRange (IEnumerable< int > values)
     *                   AddRange (IEnumerable< long > values)
     *                   AddRange (IEnumerable< ObjectId > values)
     *                   AddRange (IEnumerable< string > values)
     *                   AddRange (IEnumerable values)
     *                   1.创建不同的数组，通过C#端插入该数组数据，分别通过不同方式创建数组
     *                   2.查询插入数据是否正确
     * testcase:     seqDB-14618
     * author:       chensiqin
     * date:         2019/03/15
    */

    [TestClass]
    public class TestBsonArray14618
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14618";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14618()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            BsonDocument doc = new BsonDocument("test", 1);

            
            List<bool> boolList = new List<bool> { true, false };
            
            List<BsonValue> bsonValueList = new List<BsonValue> { doc.GetElement("test").Value };
            
            List<DateTime> dateTimeList = new List<DateTime> { new DateTime(2016, 9, 1), new DateTime(2016, 9, 2) };
            
            List<double> doubleList = new List<double> { 0.12, 0.34 };
            
            List<int> intList = new List<int> { 12, 24 };
            
            List<long> longList = new List<long> { long.MaxValue, long.MinValue };
           
            List<ObjectId> objectIdList = new List<ObjectId> { new ObjectId("123456789012345678901234"), new ObjectId("123456789012345678901233") };
            
            List<string> stringList = new List<string> { "sequoiadb", "sequoiacm" };

            List<BsonDocument> docList = new List<BsonDocument>
            {
                new BsonDocument("sequoiadb1", "sequoiadb"),
                new BsonDocument("sequoiadb2", 1)
            };
            int[] arr = new int[]{1,2,3};
            BsonArray ba = new BsonArray(3);
            Assert.AreEqual(3, ba.Capacity);
            BsonDocument record = new BsonDocument()
                .Add("arr1", new BsonArray())
                .Add("arr2", new BsonArray(boolList))
                .Add("arr3", new BsonArray(bsonValueList))
                .Add("arr4", new BsonArray(dateTimeList))
                .Add("arr5", new BsonArray(doubleList))
                .Add("arr6", new BsonArray(intList))
                .Add("arr7", new BsonArray(longList))
                .Add("arr8", new BsonArray(objectIdList))
                .Add("arr9", new BsonArray(stringList))
                .Add("arr10", new BsonArray(docList))
                .Add("arr11", new BsonArray(arr))
                .Add("arr12", new BsonArray().Add(doc.GetElement("test").Value))
                .Add("arr13", new BsonArray().AddRange(boolList))
                .Add("arr14", new BsonArray().AddRange(bsonValueList))
                .Add("arr15", new BsonArray().AddRange(dateTimeList))
                .Add("arr16", new BsonArray().AddRange(doubleList))
                .Add("arr17", new BsonArray().AddRange(intList))
                .Add("arr18", new BsonArray().AddRange(longList))
                .Add("arr19", new BsonArray().AddRange(objectIdList))
                .Add("arr20", new BsonArray().AddRange(stringList))
                .Add("arr21", new BsonArray().AddRange(docList));

            cl.Insert(record);

            DBCursor cur = cl.Query();
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument bsonDoc = cur.Current();
                Assert.AreEqual(record, bsonDoc);
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
