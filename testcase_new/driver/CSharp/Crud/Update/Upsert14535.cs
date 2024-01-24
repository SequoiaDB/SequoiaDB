using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Update
{
    /**
     * description: upsert all records
     *              interface: Upsert (BsonDocument matcher, BsonDocument modifier, BsonDocument hint)
     * testcase:    14535
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class Upsert14535
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl14535";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
        }
        
        [TestMethod()]
        public void TestUpsert14535()
        {
            BsonDocument doc = new BsonDocument();
            int[] intArr = {1, 2, 3, 4};
            doc.Add("arr", new BsonArray(intArr));
            doc.Add("str", "tom");
            cl.Insert(doc);

            // test $pull
            BsonDocument modifier = new BsonDocument("$pull", new BsonDocument("arr", 2));
            BsonDocument updatedDoc = UpsertAndReturn(cl, modifier);
            ((BsonArray)doc.GetValue("arr")).Remove(2);
            Assert.IsTrue(Common.IsEqual(doc, updatedDoc));

            // test $pop
            modifier = new BsonDocument("$pop", new BsonDocument("arr", 1));
            updatedDoc = UpsertAndReturn(cl, modifier);
            ((BsonArray)doc.GetValue("arr")).Remove(4);
            Assert.IsTrue(Common.IsEqual(doc, updatedDoc));
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();    
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }

        // upsert and then return the first record.
        public BsonDocument UpsertAndReturn(DBCollection cl, BsonDocument modifier)
        {
            BsonDocument matcher = null;
            BsonDocument hint = null;
            cl.Upsert(matcher, modifier, hint);

            DBCursor cursor = cl.Query();
            BsonDocument res = cursor.Next();
            cursor.Close();
            return res;
        }

    }
}
