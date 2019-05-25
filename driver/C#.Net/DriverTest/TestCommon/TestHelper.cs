using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace DriverTest.TestCommon
{
    public class TestHelper
    {
        
        public static bool ByteArrayEqual(byte[] left, byte[] right)
        {
            if (left == null && right == null)
            {
                return true;
            }
            if (left == null || right == null)
            {
                return false;
            }
            if (left.Length != right.Length)
            {
                return false;
            }
            int compareLength = left.Length;
            for (int i = 0; i < compareLength; i++)
            {
                if (left[i] != right[i])
                {
                    return false;
                }
            }
            return true;
        }

        public static int ByteArrayCompare(byte[] left, byte[] right)
        {
            if (left == null && right == null)
            {
                return 0;
            }
            if (left != null && right == null)
            {
                return 1;
            }
            if (left == null && right != null)
            {
                return -1;
            }
            int compareLength = Math.Min(left.Length, right.Length);
            for (int i = 0; i < compareLength; i++)
            {
                if (left[i] > right[i])
                {
                    return 1;
                }
                else if (left[i] < right[i])
                {
                    return -1;
                }
            }
            if (left.Length - compareLength > 0)
            {
                return 1;
            }
            if (right.Length - compareLength > 0)
            {
                return -1;
            }
            return 0;
        }
    }
}
