package com.sequoiadb.test.common;

import org.bson.types.BSONDecimal;

import java.util.Random;

public class DecimalCommon {
    public static int MAX_PRECISION = 1000;
    public static int INVALID_PRECISION = -1;
    public static int INVALID_SCALE = -1;
    public static Random rand = new Random();

    private static int _genPrecision() {
        int retPrecision = 0;
        Random rand = new Random();
        retPrecision = rand.nextInt(1000) + 1; // [1, 1000]
        return retPrecision;
    }

    private static int _genScale(int precision) {
        int retScale = 0;
        Random rand = new Random();
        retScale = rand.nextInt(precision + 1); // [0, precision)
        return retScale;
    }

    private static int _genIntegerDigitsNum(int precision, int scale) {
        int retNum = 0;
        Random rand = new Random();
        if (scale > precision) {
            throw new IllegalArgumentException("scale can't be great than precision");
        }
        int integerPart = precision - scale;
        if (integerPart == 0) {
            retNum = 0;
        } else {
            retNum = rand.nextInt(integerPart) + 1; // [1, integerPart]
        }
        return retNum;
    }

    public static BSONDecimal genBSONDecimal() {
        BSONDecimal retDecimal = null;

        Random rand = new Random();
        int precision = rand.nextInt(1000) + 1; // [1, 1000]
        int scale = rand.nextInt(precision); // [0, precision)

        int integerPart = precision - scale;
        int intPart = rand.nextInt(integerPart) + 1; // (0, integerPart]
        int decimalPart = rand.nextInt(scale + 1); // [0, scale]
        int firstDigit = (intPart != 1) ? (rand.nextInt(9) + 1) : rand.nextInt(10);

        String intPartStr = "" + firstDigit;
        String decimalPartStr = "";
        int tmpNum = intPart;
        while (--tmpNum > 0) {
            intPartStr += rand.nextInt(10);
        }
        tmpNum = decimalPart;
        while (tmpNum-- > 0) {
            decimalPartStr += rand.nextInt(10);
        }
        String targetValue = (decimalPartStr != "") ? (intPartStr + "." + decimalPartStr) : (intPartStr);
        targetValue = rand.nextBoolean() ? targetValue : ("-" + targetValue);
        retDecimal = new BSONDecimal(targetValue, precision, scale);

        return retDecimal;
    }

    public static BSONDecimal genBSONDecimal(boolean hasPrecision, boolean hasIntPart, boolean hasE, int eNum) {
        BSONDecimal retDecimal = null;
        String targetValue = "";
        int maxPrecision = 0;
        int maxScale = 0;
        int precision = 0;
        int scale = 0;
        int intPartDigits = 0;
        int scalePartDigits = 0;
        int firstDigit = 0;
        Random rand = new Random();

        if (hasPrecision) {
            if (hasE) {
                if ((eNum > 0 && (eNum > MAX_PRECISION)) ||
                    (eNum < 0 && (eNum < -MAX_PRECISION))) {
                    throw new IllegalArgumentException("the range of exponent should be [-1000, 1000]");
                }
            }
            precision = _genPrecision();
            scale = _genScale(precision);
            intPartDigits = _genIntegerDigitsNum(precision, scale);
            if (hasIntPart && intPartDigits > 0) {
                firstDigit = (intPartDigits != 1) ? (rand.nextInt(9) + 1) : rand.nextInt(10);
                targetValue += firstDigit;
                while (--intPartDigits > 0) {
                    targetValue += rand.nextInt(10);
                }
                if (scale > 0) {
                    targetValue += ".";
                }
            } else {
                targetValue += ".";
            }
            scalePartDigits = scale;
            while (scalePartDigits-- > 0) {
                targetValue += rand.nextInt(10);
            }
            if (hasE) {
                targetValue += "E" + eNum;
            }
            targetValue = rand.nextBoolean() ? ("-" + targetValue) : targetValue;
            retDecimal = new BSONDecimal(targetValue, precision, scale);
        } else {
            intPartDigits = rand.nextInt(3 * MAX_PRECISION) + 1;
            scalePartDigits = rand.nextInt(3 * MAX_PRECISION);
            if (hasIntPart) {
                firstDigit = (intPartDigits != 1) ? (rand.nextInt(9) + 1) : rand.nextInt(10);
                targetValue += firstDigit;
                while (--intPartDigits > 0) {
                    targetValue += rand.nextInt(10);
                }
                if (scalePartDigits > 0) {
                    targetValue += ".";
                }
            } else {
                if (scalePartDigits == 0) {
                    targetValue += ".0";
                } else {
                    targetValue += ".";
                }
            }

            while (scalePartDigits-- > 0) {
                targetValue += rand.nextInt(10);
            }
            if (hasE) {
                targetValue += "E" + eNum;
            }
            targetValue = rand.nextBoolean() ? ("-" + targetValue) : targetValue;
            retDecimal = new BSONDecimal(targetValue);
        }

        return retDecimal;
    }

    public static BSONDecimal genIntegerBSONDecimal(boolean withPrecision, boolean withScale) {
        if (!withPrecision && withScale) {
            throw new IllegalArgumentException("wrong combination of the input arguments");
        }
        BSONDecimal retDecimal = genBSONDecimal();
        String intPart = retDecimal.getValue().split("\\.")[0];
        if (withPrecision && withScale) {
            int precision = retDecimal.getPrecision();
            int scale = retDecimal.getScale();
            retDecimal = new BSONDecimal(intPart, precision, scale);
        } else if (withPrecision && !withScale) {
            int precision = retDecimal.getPrecision();
            retDecimal = new BSONDecimal(intPart, precision, 0);
        } else {
            retDecimal = new BSONDecimal(intPart);
        }
        return retDecimal;
    }


}
