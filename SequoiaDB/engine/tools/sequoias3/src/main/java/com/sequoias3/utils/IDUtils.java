package com.sequoias3.utils;

import java.util.Random;

public class IDUtils {

    public static String getAccessKeyID() {
        Random random = new Random();
        StringBuffer accessKey = new StringBuffer();
        String[] charSet = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
                "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
                "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

        for (int index = 0; index < 20; index++) {
            int number = random.nextInt(charSet.length);
            accessKey.append(charSet[number]);
        }
        return accessKey.toString();
    }

    public static String getSecretKey() {
        Random random = new Random();
        StringBuffer secretAccessKey = new StringBuffer();
        String[] charSet = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
                "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
                "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
                "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
                "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

        for (int index = 0; index < 40; index++) {
            int number = random.nextInt(charSet.length);
            secretAccessKey.append(charSet[number]);
        }
        return secretAccessKey.toString();
    }
}
