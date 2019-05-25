package com.sequoiadb.test.common;

import org.junit.Assert;

import java.io.*;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class LobHelper {
    public static String getMD5(String inputFile) throws IOException {
        String md5 = null;
        int bufferSize = 256 * 1024;
        FileInputStream fileInputStream = null;
        DigestInputStream digestInputStream = null;

        try {
            MessageDigest messageDigest = MessageDigest.getInstance("MD5");

            fileInputStream = new FileInputStream(inputFile);
            digestInputStream = new DigestInputStream(fileInputStream, messageDigest);

            byte[] tmpBuffer = new byte[bufferSize];
            while (digestInputStream.read(tmpBuffer) > 0) {
                ; // do nothing
            }

            byte[] resultByteArray = messageDigest.digest();
            return byteArrayToHex(resultByteArray);
        } catch (NoSuchAlgorithmException e) {
            return null;
        } finally {
            if (digestInputStream != null) {
                digestInputStream.close();
            }

            if (fileInputStream != null) {
                fileInputStream.close();
            }
        }
    }

    public static String byteArrayToHex(byte[] byteArray) {
        char[] hexDigits = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
        char[] resultCharArray = new char[byteArray.length * 2];
        int index = 0;
        for (byte b : byteArray) {
            resultCharArray[index++] = hexDigits[b >> 4 & 0xf];
            resultCharArray[index++] = hexDigits[b & 0xf];
        }
        return new String(resultCharArray);
    }

    public static void deleteOnExist(String fileName) {
        File file = new File(fileName);
        file.deleteOnExit();
    }

    public static void genFile(String fileName, int fileSize, String filePath) {
        String fileFullPath = null;
        int MAX_NUM = 1024 * 1024;
        if (fileName == null || fileSize < 0) {
            throw new IllegalArgumentException("invalid arguments");
        }
        if (filePath == null) {
            fileFullPath = fileName;
        } else {
            if (System.getProperty("os.name").startsWith("Windows")) {
                fileFullPath += "\\" + fileName;
            } else {
                fileFullPath += "/" + fileName;
            }
        }
        File file = new File(fileFullPath);
        if (file.exists()) {
            if (!file.delete()) {
                Assert.fail();
            }
        }
        FileOutputStream fos = null;
        try {
            try {
                fos = new FileOutputStream(fileFullPath);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
                Assert.fail();
            }
            int leftNum = fileSize;
            byte[] bytes = null;
            while (leftNum > 0) {
                if (leftNum > MAX_NUM) {
                    if (bytes == null) {
                        bytes = new byte[MAX_NUM];
                        for (int i = 0; i < bytes.length; ++i) {
                            bytes[i] = 'a';
                        }
                    }
                } else {
                    bytes = new byte[leftNum];
                    for (int i = 0; i < bytes.length; ++i) {
                        bytes[i] = 'a';
                    }
                }
                try {
                    fos.write(bytes);
                } catch (IOException e) {
                    e.printStackTrace();
                    Assert.fail();
                }
                leftNum -= bytes.length;
            }
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                    Assert.fail();
                }
            }
        }
    }
}
