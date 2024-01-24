using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Monitor
{
    /**
    * description: get snapshot of the index stats                
    * testcase:    seqDB-22774
    * author:       liuli
    * date:         2020/09/24
    */
    [TestClass]
    public class TestSnapshotIndexstats22774
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string csName = "cs22774";
        private const string clName = "cl22774";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine( DateTime.Now.ToString( "yyyy-MM-dd hh:mm:ss:fff" ) + " begin: " + this.GetType().ToString() );
            sdb = new Sequoiadb( SdbTestBase.coordUrl );
            sdb.Connect();
            cs = sdb.CreateCollectionSpace( csName );
            cl = cs.CreateCollection( clName );
        }

        [TestMethod]
        public void TestIndexstatsSnapshot22774()
        {
            String indexName = "index_22774";
            DBCursor cursor = null;
            const string clFullName = csName + "." + clName;
            BsonDocument indexDef = new BsonDocument( "a", 1 );
            List<BsonDocument> insertor = new List<BsonDocument>();
            
            for ( int i = 0; i < 3; i++ )
            {
                BsonDocument obj = new BsonDocument();
                obj.Add( "a", i );
                insertor.Add( obj );
            }
            
            cl.BulkInsert( insertor, 0 );
            cl.CreateIndex( indexName, indexDef, true, false );
            sdb.Analyze( new BsonDocument { { "Collection", clFullName }, { "Index", indexName } } );

            BsonDocument selector = new BsonDocument();
            selector.Add( "Collection", 1 );
            selector.Add( "Index", 1 );
            selector.Add( "KeyPattern", 1 );
            cursor = sdb.GetSnapshot( SDBConst.SDB_SNAP_INDEXSTATS, new BsonDocument( "Index", indexName ), selector, null );

            BsonDocument actObj = cursor.Current();
            cursor.Close();
            BsonDocument expect = new BsonDocument();
            expect.Add("Collection", clFullName);
            expect.Add( "Index", indexName );
            expect.Add( "KeyPattern", indexDef );

            Assert.AreEqual( expect, actObj );
        }

        [TestCleanup()]
        public void TearDown()
        {
            if ( sdb.IsCollectionSpaceExist( csName ) )
            {
                sdb.DropCollectionSpace( csName );
            }
            if ( sdb != null )
            {
                sdb.Disconnect();
            }
            Console.WriteLine( DateTime.Now.ToString( "yyyy-MM-dd hh:mm:ss:fff" ) + " end  : " + this.GetType().ToString() );
        }

    }
}
