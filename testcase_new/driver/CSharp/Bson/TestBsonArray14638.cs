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
     *                    Clone ()
     *                    Clear ()
     *                    Equals ( Object obj)
     *                    Equals (BsonArray rhs)
     *                    CompareTo (BsonArray other)
     *                    CompareTo (BsonValue other)
     *                    GetHashCode ()
     *                    Contains (BsonValue value)
     *                    CopyTo (BsonValue[] array, int arrayIndex)
     *                    CopyTo (object[] array, int arrayIndex)
     *                    DeepClone ()
     *                    GetEnumerator ()
     *                    IndexOf (BsonValue value)
     *                    IndexOf (BsonValue value, int index)
     *                    IndexOf (BsonValue value, int index)
     *                    Insert (int index, BsonValue value)
     *                    Remove (BsonValue value)
     *                    RemoveAt (int index)
     *                    ToArray ()
     *                    ToList ()
     *                    ToString ()
     *                    WriteTo (BsonWriter bsonWriter)
     * testcase:     seqDB-14638
     * author:       chensiqin
     * date:         2019/03/15
    */
    [TestClass]
    public class TestBsonArray14638
    {
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14638()
        {
            BsonArray arr = new BsonArray().Add(1).Add(2).Add("123");
            BsonArray arr2 = new BsonArray().Add(2).Add(2).Add("123");

            BsonValue clone = arr.Clone();
            Assert.AreEqual(true, arr.Equals(clone));
            Assert.AreEqual(false, arr.Equals(arr2));
            Assert.AreEqual(-1, arr.CompareTo(arr2));
            Assert.AreEqual(0, arr.CompareTo(clone));
            Assert.AreEqual(arr.GetHashCode(), arr.GetHashCode());
            Assert.AreEqual(arr.GetHashCode(), clone.GetHashCode());
            Assert.AreNotEqual(arr.GetHashCode(), arr2.GetHashCode());
            Assert.AreEqual(true, arr.Contains(1));

            BsonValue[] barr = new BsonValue[3];
            arr.CopyTo(barr, 0);
            Assert.AreEqual(1, barr[0]);
            Assert.AreEqual(2, barr[1]);
            Assert.AreEqual("123", barr[2]);

            object[] oarr = new object[3];
            arr.CopyTo(oarr, 0);
            Assert.AreEqual(1, oarr[0]);
            Assert.AreEqual(2, oarr[1]);
            Assert.AreEqual("123", oarr[2]);

            BsonValue dclone = arr.DeepClone();
            Assert.AreEqual(true, arr.Equals(dclone));

            IEnumerator<BsonValue> ienumber = arr.GetEnumerator();
            BsonArray expected = new BsonArray();
            while (ienumber.MoveNext())
            {
                expected.Add(ienumber.Current);
            }
            Assert.AreEqual(true, arr.Equals(expected));

            Assert.AreEqual(0, arr.IndexOf(1));
            Assert.AreEqual(-1, arr.IndexOf(1, 1));
            Assert.AreEqual(1, arr.IndexOf(2, 1, 1));

            arr.Insert(0, "sequoiadb");
            expected = new BsonArray().Add("sequoiadb").Add(1).Add(2).Add("123");
            Assert.AreEqual(true, arr.Equals(expected));

            arr.Remove("123");
            expected = new BsonArray().Add("sequoiadb").Add(1).Add(2);
            Assert.AreEqual(true, arr.Equals(expected));

            arr.RemoveAt(1);
            expected = new BsonArray().Add("sequoiadb").Add(2);
            Assert.AreEqual(true, arr.Equals(expected));

            barr = new BsonValue[2];
            barr = arr.ToArray();
            Assert.AreEqual("sequoiadb", barr[0]);
            Assert.AreEqual(2, barr[1]);

            List<BsonValue> list = arr.ToList();
            IEnumerator<BsonValue> it = list.GetEnumerator();
            int count = 0;
            expected = new BsonArray();
            while (it.MoveNext())
            {
                count++;
                expected.Add(it.Current);
            }
            Assert.AreEqual(true, arr.Equals(expected));
            Assert.AreEqual(2, count);

            Assert.AreEqual("[sequoiadb, 2]", arr.ToString());
           
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
