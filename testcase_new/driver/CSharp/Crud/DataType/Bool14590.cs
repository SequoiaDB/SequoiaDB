using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;
using System.Text.RegularExpressions;

namespace CSharp.Crud.DataType
{
    /**
     * description: test type bool
     * testcase:    14590
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class Bool14590
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl14590";

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
        public void TestBool14590()
        {
            BsonDocument oldDoc = new BsonDocument{ { "a", BsonBoolean.True }, { "b", BsonBoolean.False } };
            cl.Insert(oldDoc);
            
            BsonDocument newDoc = new BsonDocument{ { "a", BsonBoolean.False }, { "b", BsonBoolean.True } };
            BsonDocument modifier = new BsonDocument("$set", newDoc);
            cl.Update(/*matcher*/null, modifier, /*hint*/null);
            
            DBCursor cursor = cl.Query();
            BsonDocument readDoc = cursor.Next();
            Assert.AreEqual(null, cursor.Next());
            cursor.Close();
            
            readDoc.Remove("_id");
            Assert.AreEqual(newDoc, readDoc);

            cl.Delete(null);
            Assert.AreEqual(0, cl.GetCount(null));
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
    }
}
