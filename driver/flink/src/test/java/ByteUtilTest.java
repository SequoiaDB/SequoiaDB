
import com.sequoiadb.flink.common.util.ByteUtil;

import org.junit.Assert;
import org.junit.Test;

public class ByteUtilTest {
    @Test
    public void toBytesBooleantest(){
        boolean boolTest1 = true;
        boolean boolTest0 = false;

        byte[] expect1 = new byte[]{1};
        byte[] expect0 = new byte[]{0};

        Assert.assertArrayEquals(expect1, ByteUtil.toBytes(boolTest1));
        Assert.assertArrayEquals(expect0, ByteUtil.toBytes(boolTest0)); 
    }

    @Test
    public void toBytesShorttest1(){
        short shortValue = 0;
        
        byte b1 = (byte) 0;
        byte b2 = (byte) 0;

        byte[] expected = new byte[]{b1, b2};

        Assert.assertArrayEquals(expected, ByteUtil.toBytes(shortValue));
    }

    @Test
    public void toBytesShorttest2(){
        short shortValue = (short) 0xDEAD;
        
        byte b1 = (byte) 0xAD;
        byte b2 = (byte) 0xDE;

        byte[] expected = new byte[]{b1, b2};

        Assert.assertArrayEquals(expected, ByteUtil.toBytes(shortValue));
    }

    @Test
    public void toBytesInttest1(){
        int intValue = (int) 0;
        
        byte b1 = (byte) 0;
        byte b2 = (byte) 0;
        byte b3 = (byte) 0;
        byte b4 = (byte) 0;

        byte[] expected = new byte[]{b1, b2, b3, b4};

        Assert.assertArrayEquals(expected, ByteUtil.toBytes(intValue));
    }

    @Test
    public void toBytesInttest2(){
        int intValue = (int) 0xDEADBEEF;
        
        byte b1 = (byte) 0xEF;
        byte b2 = (byte) 0xBE;
        byte b3 = (byte) 0xAD;
        byte b4 = (byte) 0xDE;

        byte[] expected = new byte[]{b1, b2, b3, b4};

        Assert.assertArrayEquals(expected, ByteUtil.toBytes(intValue));
    }
    
    @Test
    public void toBytesLongtest1(){
        long longValue = (long) 0;
        
        byte b1 = (byte) 0;
        byte b2 = (byte) 0;
        byte b3 = (byte) 0;
        byte b4 = (byte) 0;
        byte b5 = (byte) 0;
        byte b6 = (byte) 0;
        byte b7 = (byte) 0;
        byte b8 = (byte) 0;

        byte[] expected = new byte[]{b1, b2, b3, b4, b5, b6, b7, b8};

        Assert.assertArrayEquals(expected, ByteUtil.toBytes(longValue));
    }
  
    @Test
    public void toBytesLongtest2(){
        long longValue = -2401053088876216593L;
        
        byte b1 = (byte) 0xEF;
        byte b2 = (byte) 0xBE;
        byte b3 = (byte) 0xAD;
        byte b4 = (byte) 0xDE;
        byte b5 = (byte) 0xEF;
        byte b6 = (byte) 0xBE;
        byte b7 = (byte) 0xAD;
        byte b8 = (byte) 0xDE;

        byte[] expected = new byte[]{b1, b2, b3, b4, b5, b6, b7, b8};

        Assert.assertArrayEquals(expected, ByteUtil.toBytes(longValue));
    }

    
}
