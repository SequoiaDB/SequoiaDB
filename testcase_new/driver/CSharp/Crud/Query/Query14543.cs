using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Query
{
    /**
     * description: query with complex selector
     *              interface: Query (BsonDocument query，BsonDocument selector，BsonDocument orderBy，
     *                                BsonDocument hint，long skipRows，long returnRows)
     * testcase:    14543
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class Query14543
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl14543";

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
        public void TestQuery14543()
        {
            BsonBinaryData bigValue = new BsonBinaryData(new byte[3 * 1024 * 1024]); // in this driver, bson max doc size is 4M
            List<BsonDocument> insertor = new List<BsonDocument>()
            {
                BsonDocument.Parse("{ complexObj: { subObj1 : { subSubObj: 1 }, subObj2: 1 }, " + 
                    "complexArray: [ { subObj: 1 }, [ 1, 2, 3 ] ] }"),
                new BsonDocument(),
                new BsonDocument("bigDoc", bigValue)
            };
            cl.BulkInsert(insertor, 0);

            BsonDocument selector = BsonDocument.Parse("{ bigDoc: { $default: 'default value' }, " + 
                   " complexObj: { $elemMatch: { subObj1: { $elemMatch: { subSubObj: 1 } } } }, " + 
                   " complexArray: { $elemMatchOne: { subObj: 1 } } }");
            long returnRows = -1;
            List<BsonDocument> queryDocs = QueryAndReturnDoc(cl, /*matcher*/null, selector, /*orderBy*/null, 
                /*hint*/null, /*skipRows*/0, returnRows);
            List<BsonDocument> expDocs = new List<BsonDocument>()
            {
                BsonDocument.Parse("{ 'complexObj': { 'subObj1': { 'subSubObj' : 1 } }, " + 
                    "'complexArray' : [{ 'subObj' : 1 }], " + 
                    "'bigDoc' : 'default value' }"),
                new BsonDocument("bigDoc", "default value"),
                new BsonDocument("bigDoc", bigValue)
            };
            Assert.IsTrue(Common.IsEqual(expDocs, queryDocs),
                "expDocs:" + expDocs.ToJson() + " actDocs:" + queryDocs.ToJson());

            returnRows = 0;
            queryDocs = QueryAndReturnDoc(cl, /*matcher*/null, selector, /*orderBy*/null, 
                /*hint*/null, /*skipRows*/0, returnRows);
            Assert.AreEqual(0, queryDocs.Count, "query should return nothing, but get " + queryDocs.ToJson());
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

        private List<BsonDocument> QueryAndReturnDoc(DBCollection cl,
                                                     BsonDocument matcher, 
                                                     BsonDocument selector,
                                                     BsonDocument orderBy,
                                                     BsonDocument hint,
                                                     long skipRows,
                                                     long returnRows
                                                     )
        {
            List<BsonDocument> result = new List<BsonDocument>();
            DBCursor cursor = cl.Query(matcher, selector, orderBy, hint, skipRows, returnRows);
            while (cursor.Next() != null)
            {
                result.Add(cursor.Current());
            }
            cursor.Close();
            return result;
        }
    }
}
