/*
 * Copyright 2012-2017 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package org.bson.types;

import java.io.Serializable;
import java.math.BigDecimal;

import org.bson.BSON;
import org.bson.util.JSON;

/**
 * The type <code>BSONDecimal</code> object can store numbers with a very large
 * number of digits.
 * <p>
 * We use the following terms below: The <code>scale</code> of a
 * <code>BSONDecimal</code> is the max number of digits after the decimal point. 
 * The <code>precision</code> of a <code>BSONDecimal</code> is the total count 
 * of significant digits in the whole number,
 * that is, the number of digits to both sides of the decimal point. So the
 * number 23.5141 has a <code>precision</code> of 6 and a <code>scale</code> of 4. 
 * Integers can be considered to have a <code>scale</code> of zero. Besides,
 * when user does not specified <code>precision</code> and <code>scale</code> to
 * build a <code>BSONDecimal</code> object, set both of them as -1. That means
 * the <code>precision</code> and <code>scale</code> are determined by the upper
 * limit of accuracy of the database. Database allow 131072 digits before decimal 
 * point and 16383 digits after decimal point at most.
 * </p>
 * <p>
 * By the way, when user specified the <code>precision</code> and <code>scale</code>, 
 * if the fractional part of the offered decimal is greater the
 * <code>scale</code>, BSONDecimal will round the fractional part to the
 * specified <code>scale</code> with the mode of HALP_UP.
 * </p>
 * Here is a example:
 * 
 * <pre>
 * BSONDecimal decimal1 = new BSONDecimal(&quot;123.456789&quot;, 10, 5);
 * BSONDecimal decimal2 = new BSONDecimal(&quot;-123.456789&quot;, 10, 5);
 * System.out.println(decimal1.toString()); // shows 123.45679;
 * System.out.println(decimal2.toString()); // shows -123.45679;
 * </pre>
 * 
 */
public class BSONDecimal implements Comparable<BSONDecimal>, Serializable {

	private static final long serialVersionUID = 8374882369691286577L;

	private static final int DECIMAL_SIGN_MASK = 0xC000;
	private static final int SDB_DECIMAL_POS = 0x0000;
	private static final int SDB_DECIMAL_NEG = 0x4000;
	private static final int SDB_DECIMAL_SPECIAL_SIGN = 0xC000;

	private static final int SDB_DECIMAL_SPECIAL_NAN = 0x0000;
	private static final int SDB_DECIMAL_SPECIAL_MIN = 0x0001;
	private static final int SDB_DECIMAL_SPECIAL_MAX = 0x0002;

	private static final int DECIMAL_DSCALE_MASK = 0x3FFF;

	/*
	 * Hardcoded precision limit - arbitrary, but must be small enough that
	 * dscale values will fit in 14 bits.
	 */
	private static final int DECIMAL_MAX_PRECISION = 1000;
	private static final int DECIMAL_MAX_DWEIGHT = 131072;
	private static final int DECIMAL_MAX_DSCALE = 16383;

	private static final int DECIMAL_NBASE = 10000;
	private static final int DECIMAL_HALF_NBASE = 5000;
	private static final int DECIMAL_DEC_DIGITS = 4; /* decimal digits per NBASE digit */

	private static final int SIZEOF_SHORT = Short.SIZE/Byte.SIZE;

	private String _value;
	private int _precision;
	private int _scale;

    private int _mid_typemod;
    private int _mid_ndigits;
    private int _mid_sign;
    private int _mid_dscale;
    private int _mid_weight;
    private short[] _mid_digits;
    private int _mid_digits_idx = 0;
    private boolean _hasCarry = false;

    private int _size;
    private int _typemod;
    private short _signscale;
    private short _weight;
    private short[] _digits;

    private static final int[] round_powers = { 0, 1000, 100, 10 };

    /*
     * The size of a empty decimal(only has header: 
     * "size + typemod + dscale + weight") 
     */ 
    public static final int DECIMAL_HEADER_SIZE = 12;
	
    /**
	 * Get the value of decimal.
	 * @return the value of decimal
	 */
	public String getValue() {
		return _value;
	}

	/**
	 * Get the precision of decimal.
	 * When user specify precision, the range of it is [1, 1000]. When user did not specify
     * 	precision, it will be set to -1. That means the precision is determined 
     * 	by database. For decimal, database allow 131072 digits before decimal point and 16383 digits
     * 	after decimal point at most. 
	 * @return return the precision specified by user or -1 for
	 *         user did not specify it.
	 */
	public int getPrecision() {
		return _precision;
	}

	/**
	 * Get the scale of the decimal.
	 * When user specify scale, the range of it is [0, precision].
     * 	When user did not specify scale, it will be set to -1. 
     * 	That means the scale is determined 
     * 	by database. For decimal, database allow 131072 digits before decimal point and 16383 digits
     * 	after decimal point at most. 
	 * @return return the scale specified by user or -1 for user
	 *         did not specify it.
	 */
	public int getScale() {
		return _scale;
	}
    
	/**
	 * @return the size
	 */
	public int getSize() {
		return _size;
	}

	/**
	 * @return the typemod
	 */
	public int getTypemod() {
		return _typemod;
	}

	/**
	 * @return the signscale
	 */
	public short getSignScale() {
		return _signscale;
	}

	/**
	 * @return the weight
	 */
	public short getWeight() {
		return _weight;
	}

	/**
	 * @return the digits
	 */
	public short[] getDigits() {
        short[] retDigits = new short[_mid_ndigits];


        if (_mid_digits != null && _mid_digits.length > 0) {
            if (!_hasCarry) {
            	System.arraycopy(_mid_digits, 1, retDigits, 0, _mid_ndigits);
            } else {
            	System.arraycopy(_mid_digits, 0, retDigits, 0, _mid_ndigits);
            }
        }
        return retDigits;
	}

	/**
	 * @param value
	 *            the decimal value which is in the format of string, e.g.
	 *            "3.14159265358"
	 * @param precision
	 *            the precision of decimal number
	 * @param scale
	 *            the total count of decimal digits in the fractional part, to
	 *            the right of the decimal point.
	 * @throws IllegalArgumentException if null pointer or invalid value
	 */
	public BSONDecimal(String value, int precision, int scale) {
		_fromStringValue(value, precision, scale);
		_value = _getValue();
		if (!_isSpecial(this)) {
			_precision = precision;
			_scale = scale;
		} else {
			_precision = -1;
			_scale = -1;		
		}
	}

	/**
	 * @param value
	 *            the decimal value which is in the format of string, e.g.
	 *            "3.14159265358"
	 * @throws IllegalArgumentException if null pointer or invalid value
	 */
	public BSONDecimal(String value) {
		this(value, -1, -1);
	}

	/**
	 * Transform a BigDecimal object to a BSONDecimal object.
	 * The meaning of "precision" and "scale" defined in BSONDecimal are
	 *       different from that defined by BigDecimal. We deprecate "precision"
	 *       and "scale" defined in BigDecimal. Actually we build BSONDecimal like
	 *       this:
	 *       <pre>
	 *       BSONDecimal(value.toString().trim()); // value is a BigDecimal object
	 *       </pre>
	 *       <p>
	 *       and set "precision" and "scale" to be -1 in the newly built BSONDecimal
	 *       object.
	 *        </p>
	 * @param value
	 *            a BigDecimal to be transformed
	 * @throws IllegalArgumentException if null pointer or invalid value
	 * @see #toBigDecimal()
	 */
	public BSONDecimal(BigDecimal value) {
		if (value == null) {
			throw new IllegalArgumentException("decimal value can't be null");
		}
		String str = value.toString();
		_fromStringValue(str, -1, -1);
		_value = _getValue();
		_precision = -1;
		_scale = -1;
	}

	/**
	 * @param size 
	 * 		total size of this decimal(4+4+2+2+digits.Length).
	 * @param typemod
	 * 		the combined precision/scale value.
     * 		precision = (typemod >> 16) & 0xffff;scale = typemod & 0xffff;
	 * @param signscale
	 * 		the combined sign/scale value.
     * 		sign = signscale & 0xC000;scale = signscale & 0x3FFF;
	 * @param weight
	 * 		weight of this decimal(NBASE=10000).
	 * @param digits
	 * 		real data.
	 * @throws IllegalArgumentException if null pointer or invalid arguments
	 */
	public BSONDecimal(int size, int typemod, 
			short signscale, short weight, short[] digits) {
        if ((size < DECIMAL_HEADER_SIZE) ||
            (size == DECIMAL_HEADER_SIZE && (digits != null && digits.length != 0)) ||
            (size > DECIMAL_HEADER_SIZE && (digits == null ||digits.length == 0))) {
            String message = String.format("Invalid arguments: size(%d), " + 
            		"typemod(%d), signscale(%d), weight(%d), digits.length(%d).",
            		size, typemod, signscale, weight, 
            		digits == null ? null : digits.length);
            throw new IllegalArgumentException(message);
        }

        if (digits == null) {
            digits = new short[0];
        }

        _size = size;
        _typemod = typemod;
        _signscale = signscale;
        _weight = weight;
        _digits = new short[digits.length + 1]; // we need another one for carry
        _digits[0] = (short)0;
        System.arraycopy(digits, 0, _digits, 1, digits.length);

        _mid_typemod = _typemod;
        _mid_ndigits = digits.length; // here we can't use "_digits.length", "digits.length" is the actual count
        _mid_sign = _signscale & DECIMAL_SIGN_MASK;
        _mid_dscale = _signscale & DECIMAL_DSCALE_MASK;
        _mid_weight = _weight;
        _mid_digits = new short[_digits.length];
        System.arraycopy(_digits, 0, _mid_digits, 0, _digits.length);
        _mid_digits_idx = 1;
        _hasCarry = false;

        _value = _getValue();
        _precision = _getPrecision();
        _scale = _getScale();
	}
	
	/**
	 * Transform to BigDecimal object
	 * The meaning of "precision" and "scale" defined in BSONDecimal are
	 * different from that defined in BigDecimal.
	 * <p>
	 * In BSONDecimal, "precision" represents the total number of digits
	 * of decimal, and "scale" represents the max count of decimal digits
	 * in the fractional part. For example, when we define a BSONDecimal
	 * object which precision is 10 and scale is 6, that means this
	 * BSONDecimal object can only represent a value which count of digits
	 * in integer part is not more than 4(10-6) and count of digits in
	 * fractional part is not more that 6. So, when we specify
	 * "12345.6789" for this BSONDecimal object, if we insert this object
	 * into database, we get an error about invalid argument. When we
	 * specify "1234.1234507" for this BSONDecimal object, if we insert
	 * this object into database, we finally get "1234.123451". We can get
	 * only 6 digits after decimal point, and the last digit is rounding
	 * to 1. when we specify "1.23E10" for this BSONDecimal object, we have
	 * 11 digits in the integer part, so when we insert this BSONDecimal
	 * object into database, we get an error, too.
	 * </p>
	 * <p>
	 * In BigDecimal, the value of the number represented by the
	 * BigDecimal is (unscaledValue * 10^-scale). "precision" represents
	 * the count of digits in unscaleValue, and "scale" is the value of
	 * scale. For example, value "1234.567890" is represented by
	 * BigDecimal like "1234567890 * 10^-6". So, unscaledValue is
	 * "1234567890", precision is 10, and scale is 6. Value "1.2345E9" is
	 * represented by BigDecimal like "12345 * 10^5". So, unscaledValue is
	 * "12345", precision is 5, and scale is -5.
	 * </p>
	 * <p>
	 * As above, we can see the difference between BSONDecimal and
	 * BigDecimal. Actually, when we transform BSONDecimal to BigDecimal,
	 * it's as below:
	 * </p>
	 *
	 * <pre>
	 * new BigDecimal(this.getValue()) // &quot;this&quot; means current BSONDecimal object
	 * </pre>
	 * @return a BigDecimal object
	 * @throws UnsupportedOperationException we can't convert a MAX/MIN/NAN value of BSONDecimal to BigDecimal.
	 */
	public BigDecimal toBigDecimal() {
		if (_isMax(this) || _isMin(this) || _isNan(this)) {
			throw new UnsupportedOperationException(String.format("can't convert %s to BigDecimal", _getValue()));
		}
		return new BigDecimal(this.getValue());
	}

	/**
	 * Whether current decimal object represents a positive infinite value
	 *
	 * @return true or false
	 */
	public boolean isMax() {
		return _isMax(this);
	}

	/**
	 * Whether current decimal object represents a negative infinite value
	 *
	 * @return true or false
	 */
	public boolean isMin() {
		return _isMin(this);
	}

	/**
	 * Whether current decimal object represents a not a number value
	 *
	 * @return true or false
	 */
	public boolean isNan() {
		return _isNan(this);
	}

    /**
     * Compares this BSONDecimal with the specified BSONDecimal.
     * Two BSONDecimal objects that are
     * equal in value(this method does not consider precision and scale,
     * so 2.0 and 2.00 are considered equal by this method).
     * This method is provided in preference to individual
     * methods for each of the six boolean
     * comparison operators (<, ==, >, >=, !=, <=).
     * The suggested idiom for performing these comparisons is:
     * (x.compareTo(y) <<i>op</i>> 0), where <<i>op</i>> is
     * one of the six comparison operators.
     * @param  other BSONDecimal to which this BSONDecimal is
     *         to be compared.
     * @return -1, 0, or 1 as this BSONDecimal is numerically 
     *         less than, equal to, or greater than val.
     */
	@Override
	public int compareTo(BSONDecimal other) {
        if (other == null) { return 1; }
        return _compareTo(other);
	}
	
    /**
     * Compares this BSONDecimal with the specified object for equality.
     * Like compareTo(BSONDecimal), this method considers two
     * BigDecimal objects equal only if they are equal in
     * value, and does not compare the precision and scale (thus
     * 2.0 is equal to 2.00 when compared by this method).
     * @param  x Object to which this BSONDecimal is to be compared.
     * @return true if and only if the specified Object is a
     *         BSONDecimal whose value is equal to this
     *         BSONDecimal's.
     * @see #compareTo(BSONDecimal)
     */
    @Override
    public boolean equals(Object x) {
        if (x == this) {
            return true;
        }
        if (!(x instanceof BSONDecimal)) {
        	return false;
        }
        if (compareTo((BSONDecimal)x) == 0) { 
        	return true;
        } else {
        	return false;
        }
    }

    /**
     * Returns the hash code for this BSONDecimal.
     * Two BSONDecimal objects that are numerically equal
     * (like 2.0 and 2.00) will generally have the same hash code.
     * @return hash code for this BSONDecimal.
     */
    @Override
    public int hashCode(){
    	String value = _value;
    	if (value.indexOf(".") > -1) {
    		value = value.replaceAll("0+$", "");
    		value = value.replaceAll("[.]$", "");
    		if (value.isEmpty()) {
    			value = "0";
    		}
    	}
        int hash = 17;
        hash = 37 * hash + 37 * BSON.NUMBER_DECIMAL;
        hash = 37 * hash + value.hashCode();
        return hash;
    }
    
    /**
     * The string value of this object.
     * @return the string value of this object
     */
    @Override
	public String toString() {
		return JSON.serialize(this);
	}
    
    /**************************** private method ****************************/
    private void _fromStringValue(String value, int precision, int scale) {
		boolean have_dp = false;
		int sign = SDB_DECIMAL_POS;
		int dweight = -1;
		int ddigits = 0;
		int dscale = 0;
		int weight = -1;
		int ndigits = 0;
		int offset = 0;

		_init(value, precision, scale);

		char[] cp = _value.toCharArray();
		int cp_idx = 0;

		if (_value.length() >= 3) {
			if ((cp[0] == 'n' || cp[0] == 'N')
					&& (cp[1] == 'a' || cp[1] == 'A')
					&& (cp[2] == 'n' || cp[2] == 'N')) {
				_setNan();
				_update();
				return;
			}
			if ((cp[0] == 'm' || cp[0] == 'M')
					&& (cp[1] == 'i' || cp[1] == 'I')
					&& (cp[2] == 'n' || cp[2] == 'N')) {
				_setMin();
				_update();
				return;
			}
			if ((cp[0] == 'm' || cp[0] == 'M')
					&& (cp[1] == 'a' || cp[1] == 'A')
					&& (cp[2] == 'x' || cp[2] == 'X')) {
				_setMax();
				_update();
				return;
			}
		}

		/*
		 * We first parse the string to extract decimal digits and determine the
		 * correct decimal weight. Then convert to NBASE representation.
		 */
		switch (cp[cp_idx]) {
		case '+':
			sign = SDB_DECIMAL_POS;
			cp_idx++;
			break;

		case '-':
			sign = SDB_DECIMAL_NEG;
			cp_idx++;
			break;
		}

		if (cp[cp_idx] == '.') {
			have_dp = true;
			cp_idx++;
		}

		if (!Character.isDigit(cp[cp_idx])) {
			throw new IllegalArgumentException("invalid decimal: " + _value);
		}
		int decdigits_idx = 0;
		int decdigitsnum = (cp.length - cp_idx) + DECIMAL_DEC_DIGITS * 2;
		int[] decdigits = new int[decdigitsnum];
		decdigits_idx = DECIMAL_DEC_DIGITS;

		while (cp_idx < cp.length) {
			if (Character.isDigit(cp[cp_idx])) {
				decdigits[decdigits_idx++] = Character.digit(cp[cp_idx++], 10);
				if (!have_dp) {
					dweight++;
					if (dweight >= DECIMAL_MAX_DWEIGHT + DECIMAL_MAX_PRECISION) {
						throw new IllegalArgumentException(
								"the integer part of the value is out of bound");
					}
				} else {
					dscale++;
					if (dscale > DECIMAL_MAX_DSCALE + DECIMAL_MAX_PRECISION) {
						throw new IllegalArgumentException(
								"the decimal part of the value is out of bound");
					}
				}
			} else if (cp[cp_idx] == '.') {
				if (have_dp) {
					throw new IllegalArgumentException(
							"invalid decimal point in decimal: " + _value);
				} else {
					have_dp = true;
					cp_idx++;
				}
			} else {
				if (cp[cp_idx] != 'e' && cp[cp_idx] != 'E') {
					throw new IllegalArgumentException(
							"invalid digit in decimal: " + _value);
				} else {
					break;
				}
			}
		}

		ddigits = decdigits_idx - DECIMAL_DEC_DIGITS;

		/* Handle exponent, if any */
		if ((cp_idx < cp.length) && (cp[cp_idx] == 'e' || cp[cp_idx] == 'E')) {
			long exponent = 0L;
			cp_idx++;
			if (cp_idx < cp.length && cp[cp_idx] == '+') {
				cp_idx++;
			}
			String strexponent = String.valueOf(cp, cp_idx, cp.length - cp_idx);
			try {
				exponent = Long.parseLong(strexponent);
			} catch (NumberFormatException e) {
				throw new IllegalArgumentException(
						"invalid exponent in decimal: " + _value);
			}
			if (exponent > DECIMAL_MAX_PRECISION
					|| exponent < -DECIMAL_MAX_PRECISION) {
				throw new IllegalArgumentException("exponent in decimal: "
						+ _value + " should be in the bound of [-1000, 1000]");
			}
			dweight += (int) exponent;
			dscale -= (int) exponent;
			if (dscale < 0) {
				dscale = 0;
			}
		}
		
		if (dweight >= DECIMAL_MAX_DWEIGHT) {
			throw new IllegalArgumentException(
					"the integer part of the value is out of bound");
		}
		if (dscale > DECIMAL_MAX_DSCALE) {
			throw new IllegalArgumentException(
					"the decimal part of the value is out of bound");				
		}
		
		/*
		 * Okay, convert pure-decimal representation to base NBASE. First we
		 * need to determine the converted weight and ndigits. offset is the
		 * number of decimal zeroes to insert before the first given digit to
		 * have a correctly aligned first NBASE digit.
		 */
		if (dweight >= 0) {
			weight = (dweight + 1 + DECIMAL_DEC_DIGITS - 1) / DECIMAL_DEC_DIGITS - 1;
		} else {
			weight = -((-dweight - 1) / DECIMAL_DEC_DIGITS + 1);
		}

		offset = (weight + 1) * DECIMAL_DEC_DIGITS - (dweight + 1);
		ndigits = (ddigits + offset + DECIMAL_DEC_DIGITS - 1)
				/ DECIMAL_DEC_DIGITS;

		_alloc(ndigits);

	    _mid_sign = sign;
	    _mid_weight = weight;
	    _mid_dscale = dscale;

		decdigits_idx = DECIMAL_DEC_DIGITS - offset;
		while (ndigits-- > 0) {
			_mid_digits[_mid_digits_idx++] = (short) (((decdigits[decdigits_idx] * 10 + decdigits[decdigits_idx + 1]) * 10 + decdigits[decdigits_idx + 2]) * 10 + decdigits[decdigits_idx + 3]);
			decdigits_idx += DECIMAL_DEC_DIGITS;
		}

		/* Strip any leading/trailing zeroes, and normalize weight if zero */
		_strip();
		
		_apply_typemod();
		
        _update();
	}

	private String _getValue() {
		short dig = 0;
		short d1 = 0;
		int d = 0;
		String retStr = null;
		
		int expect_char_size = _getExpectCharSize();
		char[] cp = new char[expect_char_size];
		int cp_idx = 0;
		int cp_idx_end = 0;
		if (_isNan(this)) {
			cp[0] = 'N';
			cp[1] = 'a';
			cp[2] = 'N';
			retStr = new String(cp);
			return retStr;
		}
		if (_isMin(this)) {
			cp[0] = 'M';
			cp[1] = 'I';
			cp[2] = 'N';
			retStr = new String(cp);
			return retStr;
		}
		if (_isMax(this)) {
			cp[0] = 'M';
			cp[1] = 'A';
			cp[2] = 'X';
			retStr = new String(cp);
			return retStr;
		}
		if (_mid_sign == SDB_DECIMAL_NEG) {
			cp[cp_idx++] = '-';
		}
		if (_mid_weight < 0) {
			d = _mid_weight + 1;
			cp[cp_idx++] = '0';
		} else {
			for (d = 0; d <= _mid_weight; d++) {
                if (_hasCarry) {
                    dig = ((d < _mid_ndigits) ? _mid_digits[d] : (short)0);
                } else {
                    dig = ((d < _mid_ndigits) ? _mid_digits[1 + d] : (short)0);
                }
				{
					boolean putit = (d > 0);

					d1 = (short) (dig / 1000);
					dig -= d1 * 1000;
					putit |= (d1 > 0);
					if (putit) {
						cp[cp_idx++] = (char) (d1 + '0');
					}

					d1 = (short) (dig / 100);
					dig -= d1 * 100;
					putit |= (d1 > 0);
					if (putit) {
						cp[cp_idx++] = (char) (d1 + '0');
					}

					d1 = (short) (dig / 10);
					dig -= d1 * 10;
					putit |= (d1 > 0);
					if (putit) {
						cp[cp_idx++] = (char) (d1 + '0');
					}

					cp[cp_idx++] = (char) (dig + '0');
				}
			}
		}
		/*
		 * If requested, output a decimal point and all the digits that follow
		 * it. We initially put out a multiple of DEC_DIGITS digits, then
		 * truncate if needed.
		 */
		if (_mid_dscale > 0) {
			cp[cp_idx++] = '.';
			cp_idx_end = cp_idx + _mid_dscale;
			for (int i = 0; i < _mid_dscale; d++, i += DECIMAL_DEC_DIGITS) {
                if (_hasCarry) {
                    dig = (d >= 0 && d < _mid_ndigits) ? _mid_digits[d] : (short)0;
                } else {
                    dig = (d >= 0 && d < _mid_ndigits) ? _mid_digits[1 + d] : (short)0;
                }
				d1 = (short)(dig / 1000);
				dig -= d1 * 1000;
				cp[cp_idx++] = (char)(d1 + '0');
				
				d1 = (short)(dig / 100);
				dig -= d1 * 100;
				cp[cp_idx++] = (char)(d1 + '0');
				
				d1 = (short)(dig / 10);
				dig -= d1 * 10;
				cp[cp_idx++] = (char)(d1 + '0');
				
				cp[cp_idx++] = (char)(dig + '0');
			}
			while(cp_idx < cp_idx_end) {
				cp[cp_idx++] = '0';
			}
			while(cp_idx_end < cp_idx) {
				cp[cp_idx_end] = (char)(cp[cp_idx_end] - '0');
				cp_idx_end++;
			}
		}
		retStr = new String(cp).trim();
		return retStr;
	}
    
	private void _init(String value, int precision, int scale) {
		if (value == null || value == "") {
			throw new IllegalArgumentException("the string of decimal value is null or empty");
		}
		if ((precision < 0 && precision != -1) || (scale < 0 && scale != -1) || 
	        (precision == -1 && scale != -1) || (scale == -1 && precision != -1 )) {
			throw new IllegalArgumentException("invalid precision: "
					+ precision + ", and scale: " + scale);
		}
		if (precision == 0 || precision > DECIMAL_MAX_PRECISION || scale > precision) {
			throw new IllegalArgumentException("invalid precision: "
					+ precision + ", and scale: " + scale);
		}
        _value = value;
        _precision = precision;
        _scale = scale;

        if (precision == -1 && scale == -1)
        {
            _mid_typemod = -1;
        }
        else
        {
            _mid_typemod = ((precision << 16) | scale);
        }
        _mid_ndigits = 0;
        _mid_sign = SDB_DECIMAL_POS;
        _mid_dscale = 0;
        _mid_weight = 0;
        _mid_digits = null;

        _size = -1;
        _typemod = -1;
        _signscale = 0;
        _weight = 0;
        _digits = null;
	}
	
	private void _setNan() {
	    _mid_ndigits = 0;
	    _mid_weight = 0;
	    _mid_sign = SDB_DECIMAL_SPECIAL_SIGN;
	    _mid_dscale = SDB_DECIMAL_SPECIAL_NAN;
	}

	private void _setMin() {
	    _mid_ndigits = 0;
	    _mid_weight = 0;
	    _mid_sign = SDB_DECIMAL_SPECIAL_SIGN;
	    _mid_dscale = SDB_DECIMAL_SPECIAL_MIN;
	}

	private void _setMax() {
	    _mid_ndigits = 0;
	    _mid_weight = 0;
	    _mid_sign = SDB_DECIMAL_SPECIAL_SIGN;
	    _mid_dscale = SDB_DECIMAL_SPECIAL_MAX;
	}
	
    private boolean _isMin(BSONDecimal value)
    {
        if (value == null)
        {
            return false;
        }
        if (_isSpecial(value) && value._mid_dscale == SDB_DECIMAL_SPECIAL_MIN)
        {
            return true;
        }
        return false;
    }
    
    private boolean _isMax(BSONDecimal value)
    {
        if (value == null)
        {
            return false;
        }
        if (_isSpecial(value) && value._mid_dscale == SDB_DECIMAL_SPECIAL_MAX)
        {
            return true;
        }
        return false;
    }
	
    private boolean _isNan(BSONDecimal value)
    {
        if (value == null)
        {
            return true;
        }
        if (_isSpecial(value) && value._mid_dscale == SDB_DECIMAL_SPECIAL_NAN)
        {
            return true;
        }
        return false;
    }

    private boolean _isSpecial(BSONDecimal value)
    {
        if (value == null)
        {
            return true;
        }
        if (value._mid_sign == SDB_DECIMAL_SPECIAL_SIGN)
        {
            return true;
        }
        return false;
    }
	
	private void _alloc(int ndigits) {
	    _mid_digits = new short[ndigits + 1]; 
	    _mid_digits_idx = 0;
	    _mid_digits[_mid_digits_idx++] = 0;
	    _mid_ndigits = ndigits;
	}
	
    private void _update()
    {
        _size = DECIMAL_HEADER_SIZE + _mid_ndigits * SIZEOF_SHORT;
        _typemod = _isSpecial(this) ? -1 : _mid_typemod;
        _signscale = (short)((_mid_dscale & DECIMAL_DSCALE_MASK) | _mid_sign);
        _weight = (short)_mid_weight;
        if (_mid_digits == null || _mid_digits.length == 0 || _mid_ndigits == 0)
        {
            _digits = _mid_digits = new short[0];
        }
        else
        {
        	_digits = _mid_digits;
        }
    }
	
	private void _strip() {
		int ndigits = _mid_ndigits;
		int digits_idx_f = 1;
		int digits_idx_e = (ndigits + 1) - 1;

		while (ndigits > 0 && _mid_digits[digits_idx_f] == 0) {
			digits_idx_f++;
			_mid_weight--;
			ndigits--;
		}

		while (ndigits > 0 && _mid_digits[digits_idx_e] == 0) {
			digits_idx_e--;
			ndigits--;
		}

		int count = digits_idx_e - digits_idx_f + 1;
		if (count != ndigits) {
			throw new IllegalStateException("ndigits[" + ndigits
					+ "] is not equal with [" + count + "]");
		}

		if (ndigits == 0) {
		    _mid_sign = SDB_DECIMAL_POS;
		    _mid_weight = 0;
		}

		if (digits_idx_f != 1) {
			for (int i = digits_idx_f, begin_idx = 1; i <= digits_idx_e; ++i, ++begin_idx) {
				_mid_digits[begin_idx] = _mid_digits[i];
				_mid_digits[i] = 0;
			}
		}
		for (int i = digits_idx_e + 1; i < _mid_digits.length; i++) {
			_mid_digits[i] = 0;
		}
	    _mid_digits_idx = 1;
	    _mid_ndigits = ndigits;
	}

	private void _apply_typemod() {
		int precision = 0;
		int scale = 0;
		int maxdigits = 0;
		int ddigits = 0;
		int i = 0;

		if (_mid_typemod == -1) {
			return;
		}

		precision = (_mid_typemod >> 16) & 0xffff;
		scale = _mid_typemod & 0xffff;
		maxdigits = precision - scale;

		_round(scale);

		/*
		 * Check for overflow - note we can't do this before rounding, because
		 * rounding could raise the weight. Also note that the var's weight
		 * could be inflated by leading zeroes, which will be stripped before
		 * storage but perhaps might not have been yet. In any case, we must
		 * recognize a true zero, whose weight doesn't mean anything.
		 */
		ddigits = (_mid_weight + 1) * DECIMAL_DEC_DIGITS;
		if (ddigits > maxdigits) {
            if (!_hasCarry) {
            	i = 1;
            } else {
            	i = 0;
            }
			for (; i <= _mid_ndigits; i++) {
				short dig = _mid_digits[i];
				if (dig != 0) {
					if (dig < 10) {
						ddigits -= 3;
					} else if (dig < 100) {
						ddigits -= 2;
					} else if (dig < 1000) {
						ddigits -= 1;
					}

					if (ddigits > maxdigits) {
						throw new IllegalArgumentException(
								"the input decimal[" + _value + "] can't be " + 
								"expressed in the range which is delimited by " +
								"the given precision[" + precision + "] and scale[" + scale + "]");
					}
					break;
				}
				ddigits -= DECIMAL_DEC_DIGITS;
			}
		}
	}

	private void _round(int rscale) {
		int di = 0;
		int ndigits = 0;
		int carry = 0;

		_mid_dscale = rscale;

		di = (_mid_weight + 1) * DECIMAL_DEC_DIGITS + rscale;

		/*
		 * If di = 0, the value loses all digits, but could round up to 1 if its
		 * first extra digit is >= 5. If di < 0 the result must be 0.
		 */
		if (di < 0) {
		    _mid_ndigits = 0;
		    _mid_weight = 0;
		    _mid_sign = SDB_DECIMAL_POS;
		} else {
			ndigits = (di + DECIMAL_DEC_DIGITS - 1) / DECIMAL_DEC_DIGITS;

			di %= DECIMAL_DEC_DIGITS;

			if ((ndigits < _mid_ndigits) || (ndigits == _mid_ndigits && di > 0)) {
				_mid_ndigits = ndigits;
				if (di == 0) {
					carry = (_mid_digits[ndigits + 1] >= DECIMAL_HALF_NBASE) ? 1
							: 0;
				} else {
					int extra = 0;
					int pow10 = 0;
					pow10 = round_powers[di];

					extra = _mid_digits[--ndigits + 1] % pow10;
					_mid_digits[ndigits + 1] -= extra;
					carry = 0;
					if (extra >= pow10 / 2) {
						pow10 += _mid_digits[ndigits + 1];
						if (pow10 >= DECIMAL_NBASE) {
							pow10 -= DECIMAL_NBASE;
							carry = 1;
						}
						_mid_digits[ndigits + 1] = (short) pow10;
					}
				}

				while (carry != 0) {
					carry += _mid_digits[--ndigits + 1];
					if (carry >= DECIMAL_NBASE) {
						_mid_digits[ndigits + 1] = (short) (carry - DECIMAL_NBASE);
						carry = 1;
					} else {
						_mid_digits[ndigits + 1] = (short) carry;
						carry = 0;
					}
				}

				if (ndigits < 0) {
					if (ndigits != -1) {
						throw new IllegalStateException("ndigits[" + ndigits
								+ "] is not -1");
					}
					if (_hasCarry) {
						throw new IllegalStateException(
								"impossible for _hasCarry to be true");
					}
					if (_mid_digits[0] == 0) {
						throw new IllegalStateException(
								"impossible for _digits[0] to be 0");
					}
					_hasCarry = true;
					_mid_ndigits++;
					_mid_weight++;
				}
			}
		}
	}
    
    private int _getPrecision() 
    {
	    if (_mid_typemod != -1) 
        {
		    return (_mid_typemod >> 16) & 0xffff;
	    } 
        else 
        {
		    return -1;
	    }
    }

    private int _getScale() 
    {
	    if (_mid_typemod != -1) 
        {
		    return _mid_typemod & 0xffff;
	    } 
        else 
        {
		    return -1;
	    }
    }
    
	private int _getExpectCharSize() {
		int retSize = 0;
		if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _mid_dscale == SDB_DECIMAL_SPECIAL_NAN) {
			return (3); // "NAN"
		}

		if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _mid_dscale == SDB_DECIMAL_SPECIAL_MIN) {
			return (3); // "MIN"
		}

		if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN
				&& _mid_dscale == SDB_DECIMAL_SPECIAL_MAX) {
			return (3); // "MAX"
		}

		/*
		 * Allocate space for the result.
		 * 
		 * tmpSize is set to the # of decimal digits before decimal point.
		 * dscale is the # of decimal digits we will print after decimal point.
		 * We may generate as many as DEC_DIGITS-1 excess digits at the end, and
		 * in addition we need room for sign, decimal point, null terminator.
		 */
		retSize = (_mid_weight + 1) * DECIMAL_DEC_DIGITS;
		if (retSize <= 0) {
			retSize = 1;
		}

		retSize += _mid_dscale + DECIMAL_DEC_DIGITS + 2;

		return retSize;
	}
	
	private int _compareTo(BSONDecimal other)
    {
        if (other == null) { return 1; }

        if (_isMin(this))
        {
            if (_isMin(other))
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else if (_isMin(other))
        {
            return 1;
        }

        if (_isMax(this))
        {
            if (_isMax(other))
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
        else if (_isMax(other))
        {
            return -1;
        }
        /*
         * postgresql's define: 
         *    We consider all NANs to be equal and larger than any non-NAN. This is
         *    somewhat arbitrary; the important thing is to have a consistent sort
         *    order.
         * 
         * while bson's define is the opposite:
         *    NAN's is smaller than any non-NAN.  bsonobj.cpp:compareElementValues
         * 
         * conclusion:  we use bson's define!
         */
        if (_isNan(this))
        {
            if (_isNan(other))
            {
                return 0; /* NAN = NAN */
            }
            else
            {
                return -1; /* NAN < non-NAN */
            }
        }
        else if (_isNan(other))
        {
            return 1; /* non-NAN > NAN */
        }

        if (this._mid_ndigits == 0)
        {
            if (other._mid_ndigits == 0)
            {
                return 0;
            }

            if (other._mid_sign == SDB_DECIMAL_NEG)
            {
                return 1;
            }

            return -1;
        }

        if (other._mid_ndigits  == 0)
        {
            if (this._mid_sign == SDB_DECIMAL_POS)
            {
                return 1;
            }

            return -1;
        }

        if (this._mid_sign == SDB_DECIMAL_POS)
        {
            if (other._mid_sign == SDB_DECIMAL_NEG)
            {
                return 1;
            }

            return _compareABS(other);
        }

        if (other._mid_sign == SDB_DECIMAL_POS)
        {
            return -1;
        }
        return -_compareABS(other);
    }
	
	 private int _compareABS(BSONDecimal other)
     {
         int i1 = 0;
         int i2 = 0;
         int weight1 = 0;
         int ndigit1 = 0;
         short[] digits1 = null;

         int weight2 = 0;
         int ndigit2 = 0;
         short[] digits2 = null;

         if (other == null)
         {
             return 1;
         }

         digits1 = this.getDigits();
         digits2 = other.getDigits();            

         if (digits1 == null || digits2 == null)
         {
             String message = "";
             if (digits1 == null)
             {
                 message = "failed to compare for no digits in current decimal.";
             }
             else
             {
                 message = "failed to compare for no digits in other decimal.";
             }
             throw new IllegalArgumentException(message);
         }

         weight1 = this.getWeight();
         ndigit1 = digits1.length;

         weight2 = other.getWeight();
         ndigit2 = digits2.length;

         /* Check any digits before the first common digit */
         while (weight1 > weight2 && i1 < ndigit1)
         {
             if (digits1[i1++] != 0)
             {
                 return 1;
             }
             weight1--;
         }

         while (weight2 > weight1 && i2 < ndigit2)
         {
             if (digits2[i2++] != 0)
             {
                 return -1;
             }

             weight2--;
         }

         /* At this point, either w1 == w2 or we've run out of digits */
         if (weight1 == weight2)
         {
             while (i1 < ndigit1 && i2 < ndigit2)
             {
                 int stat = digits1[i1++] - digits2[i2++];

                 if (stat != 0)
                 {
                     if (stat > 0)
                     {
                         return 1;
                     }

                     return -1;
                 }
             }
         }

         /*
         * At this point, we've run out of digits on one side or the other; so any
         * remaining nonzero digits imply that side is larger
         */
         while (i1 < ndigit1)
         {
             if (digits1[i1++] != 0)
             {
                 return 1;
             }
         }

         while (i2 < ndigit2)
         {
             if (digits2[i2++] != 0)
             {
                 return -1;
             }
         }

         return 0;
     }
}