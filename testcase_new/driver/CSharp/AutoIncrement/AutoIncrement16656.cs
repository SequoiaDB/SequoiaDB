using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.AutoIncrement
{
    /**
     * TestCase : seqDB-16656
     * test interface:    创建集合时指定自增字段，并修改自增字段属性 
     *                    CreateCollection (string collectionName, BsonDocument options)
     *                    Alter (BsonDocument options)
     *                    SetAttributes (BsonDocument options)
     *                   
     * author:  chensiqin
     * date:    2018/12/12
     * version: 1.0
    */

    [TestClass]
    public class AutoIncrement16656
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string clName = "cl16656";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test16656()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            CreatAutoIncreamentCL();
            AlterAutoIncreamentCL();
            SetAttributesAutoIncreamentCL();
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

        public void CreatAutoIncreamentCL(
            )
        {
            DBCollection cl = cs.CreateCollection(clName);
            List<BsonDocument> optionList = new List<BsonDocument>();

            BsonDocument options = new BsonDocument();
            options.Add("Field", "test16656");
            options.Add("MinValue", 1);
            options.Add("StartValue", 2);
            optionList.Add(options);

            cl.CreateAutoIncrement(optionList);

        }

        public void AlterAutoIncreamentCL()
        {
            //Alter (BsonDocument options)
            DBCollection cl = cs.GetCollection(clName);
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{
                           {"AutoIncrement", new BsonDocument{
                                                 {"Field","test16656"},
                                                 { "StartValue", 3}
                                             }
                           }
                       }
                 }
            });

            BsonDocument options = new BsonDocument();
            options.Add("Alter", alterArray);
            cl.Alter(options);
            List<BsonDocument> doc = GetAutoIncrement(SdbTestBase.csName + "." + clName);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", doc[0].GetElement("SequenceName").Value.ToString());
            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, matcher, new BsonDocument { { "StartValue", 1 } }, new BsonDocument());
            List<BsonDocument> expected = new List<BsonDocument>();
            List<BsonDocument> actual = new List<BsonDocument>();
            expected.Add(new BsonDocument { { "StartValue", 3 } });
            while (cur.Next() != null)
            {
                BsonDocument obj = cur.Current();
                actual.Add(obj);
            }
            cur.Close();
            Assert.AreEqual(expected.ToString(), actual.ToString());
        }

        // SetAttributes (BsonDocument options)
        public void SetAttributesAutoIncreamentCL()
        {
            DBCollection cl = cs.GetCollection(clName);
            cl.SetAttributes(new BsonDocument{
                           {"AutoIncrement", new BsonDocument{
                                                 {"Field","test16656"},
                                                 { "StartValue", 4}
                                             }
                           }
                       });
            List<BsonDocument> doc = GetAutoIncrement(SdbTestBase.csName + "." + clName);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", doc[0].GetElement("SequenceName").Value.ToString());
            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, matcher, new BsonDocument { { "StartValue", 1 } }, new BsonDocument());
            List<BsonDocument> expected = new List<BsonDocument>();
            List<BsonDocument> actual = new List<BsonDocument>();
            expected.Add(new BsonDocument { { "StartValue", 4 } });
            while (cur.Next() != null)
            {
                BsonDocument obj = cur.Current();
                actual.Add(obj);
            }
            cur.Close();
            Assert.AreEqual(expected.ToString(), actual.ToString());
        }

        public List<BsonDocument> GetAutoIncrement(string clFullName)
        {
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", clFullName);
            BsonDocument selector = new BsonDocument();
            selector.Add("AutoIncrement", 1);
            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, selector, null);
            List<BsonDocument> doc = new List<BsonDocument>();
            while (cur.Next() != null)
            {
                BsonElement element = cur.Current().GetElement("AutoIncrement");
                BsonArray arr = element.Value.AsBsonArray;
                doc.Add((BsonDocument)arr[0]);
            }
            cur.Close();
            return doc;
        }
    }
}
