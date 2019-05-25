/* Copyright 2010-2012 10gen Inc.
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace SequoiaDB.Bson
{
    /// <summary>
    /// Represents a BSON decimal value.
    /// </summary>
    /// <p>
    /// We use the following terms below: The scale of a
    /// BsonDecimal is the max number of digits after the decimal point. 
    /// The precision of a BsonDecimal is the total count 
    /// of significant digits in the whole number,
    /// that is, the number of digits to both sides of the decimal point. So the
    /// number 23.5141 has a precision of 6 and a scale of 4. 
    /// Integers can be considered to have a scale of zero. Besides,
    /// when user does not specified precision and scale to
    /// build a BsonDecimal object, set both of them as -1. That means
    /// the precision and scale are determined by the upper
    /// limit of accuracy of the database. Database allow 131072 digits before decimal 
    /// point and 16383 digits after decimal point at most.
    /// </p>
    /// <p>
    /// By the way, when user specified the precision and scale, 
    /// if the fractional part of the offered decimal is greater the
    /// scale, BsonDecimal will round the fractional part to the
    /// specified scale with the mode of HALP_UP.
    /// </p>
    /// Here is a example:
    /// 
    /// <pre>
    /// BsonDecimal decimal1 = new BsonDecimal("123.456789", 10, 5);
    /// BsonDecimal decimal2 = new BsonDecimal("-123.456789", 10, 5);
    /// Console.WriteLine(decimal1.toString()); // shows 123.45679;
    /// Console.WriteLine(decimal2.toString()); // shows -123.45679;
    /// </pre>
    [Serializable]
    public class BsonDecimal : BsonValue, IComparable<BsonDecimal>, IEquatable<BsonDecimal>
    {
        // const value
        private static readonly int DECIMAL_SIGN_MASK = 0xC000;
        private static readonly int SDB_DECIMAL_POS = 0x0000;
        private static readonly int SDB_DECIMAL_NEG = 0x4000;
        private static readonly int SDB_DECIMAL_SPECIAL_SIGN = 0xC000;

        private static readonly int SDB_DECIMAL_SPECIAL_NAN = 0x0000;
        private static readonly int SDB_DECIMAL_SPECIAL_MIN = 0x0001;
        private static readonly int SDB_DECIMAL_SPECIAL_MAX = 0x0002;

        private static readonly int DECIMAL_DSCALE_MASK = 0x3FFF;

        private static readonly int DECIMAL_MAX_PRECISION = 1000;
        private static readonly int DECIMAL_MAX_DWEIGHT = 131072;
        private static readonly int DECIMAL_MAX_DSCALE = 16383;
        private static readonly int DECIMAL_MAX_DISPLAY_SCALE = DECIMAL_MAX_PRECISION;
        //private static readonly int DECIMAL_MIN_DISPLAY_SCALE = 0;
        //private static readonly int DECIMAL_MIN_SIG_DIGITS = 16;

        private static readonly int DECIMAL_NBASE = 10000;
        private static readonly int DECIMAL_HALF_NBASE = 5000;
        private static readonly int DECIMAL_DEC_DIGITS = 4;     /* decimal digits per NBASE digit */

        // fields for define decimal
        private String _value;
        private int _precision;
        private int _scale;

        // fields for helping to build decimal(result in middle)
        private int _mid_typemod;
        private int _mid_ndigits;
        private int _mid_sign;
        private int _mid_dscale;
        private int _mid_weight;
        private short[] _mid_digits;
        private int _mid_digits_idx = 0;
        private bool _hasCarry = false;

        // fields for building bson
        private int _size;
        private int _typemod;
        private short _signscale;
        private short _weight;
        private short[] _digits;

        // other fields for help
        private static readonly int[] round_powers = { 0, 1000, 100, 10 };

        ///// <summary>
        ///// The size of a empty decimal(only has header)
        ///// </summary>
        public static readonly int DECIMAL_HEADER_SIZE = 12;

        // getter
        /// <summary>
        /// The value of decimal.
        /// </summary>
        public String Value
        {
            get { return _value; }
        }

        /// <summary>
        /// The precision specified by user or -1 for user does not specify it.
        /// It means the total number of significant digits in decimal.
        /// </summary>
        /// <remarks>
        /// When user specify precision, the range of it is [1, 1000]. When user did not specify 
        /// precision, it will be set to -1. That means the precision is determined 
        /// by database. For decimal, database allow 131072 digits before decimal point and 16383 digits
        /// after decimal point at most.
        /// </remarks>
        public int Precision
        {
            get { return _precision; }
        }

        /// <summary>
        /// The scale specified by user or -1 for user did not specify it.
        /// It means the max number of digits after the decimal point.
        /// </summary>
        /// <remarks>
        /// When user specify scale, the range of it is [0, precision]. 
        /// When user does not specify scale, it will be set to -1. 
        /// That means the scale is determined 
        /// by database. For decimal, database allow 131072 digits before decimal point and 16383 digits
        /// after decimal point at most.
        /// </remarks>
        public int Scale
        {
            get { return _scale; }
        }

        ///// <summary>
        ///// Total size of this decimal(4+4+2+2+Digits.Length).
        ///// </summary>
        public int Size
        {
            get { return _size; }
        }

        ///// <summary>
        ///// The combined precision/scale value.
        ///// precision = (typmod >> 16) & 0xffff;
        ///// scale = typmod & 0xffff;
        ///// </summary>
        public int Typemod
        {
            get { return _typemod; }
        }

        ///// <summary>
        ///// The combined sign/scale value.
        ///// sign = signscale & 0xC000;scale = signscale & 0x3FFF;
        ///// </summary>
        public short SignScale
        {
            get { return _signscale; }
        }

        ///// <summary>
        ///// Weight of this decimal(NBASE=10000).
        ///// </summary>
        public short Weight
        {
            get { return _weight; }
        }

        ///// <summary>
        ///// Real data.
        ///// </summary>
        public short[] Digits
        {
            get
            {
                short[] retDigits = new short[_mid_ndigits];

                // here _mid_digits and _digits have the same reference,
                // so we just use _mid_digits but not _digits
 
		        // when input nan/max/min, _mid_digits is null
                if (_mid_digits != null && _mid_digits.Length > 0)
                {
                    if (!_hasCarry)
                    {
				        // actually, "_ndigits" is less than "_digits.length",
				        // so, we can never user the later to replace the former
                        Array.Copy(_mid_digits, 1, retDigits, 0, _mid_ndigits);
                    } 
                    else 
                    {
				        // when we come here, we just copy "_mid_ndigits" from the 
				        // source array, but not "_mid_ndigits + 1", for we have 
				        // adjust "_mid_ndigits" when carry happen.
                        Array.Copy(_mid_digits, 0, retDigits, 0, _mid_ndigits);
                    }
                }
                return retDigits;
            }
        }

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="value">The value of the decimal in string format.</param>
        /// <param name="precision">The precision specified by user.</param>
        /// <param name="scale">The scale specified by user</param>
        public BsonDecimal(String value, int precision, int scale)
            : base(BsonType.Decimal)
        {
            _FromStringValue(value, precision, scale);
            _value = _GetValue();
            if (!_IsSpecial(this))
            {
                _precision = precision;
                _scale = scale;
            }
            else 
            {
                _precision = -1;
                _scale = -1; 
            }
        }

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="value">The value of the decimal in string format.</param>
        public BsonDecimal(String value)
            : this(value, -1, -1)
        {
        }

        /// <summary>
        ///  Constructor.
        /// </summary>
        /// <param name="value">The value of the decimal in string format.</param>
        public BsonDecimal(Decimal value)
            : base(BsonType.Decimal)
        {
            if (value == null)
            {
                if (value == null)
                {
                    throw new ArgumentException("The input decimal can not be null");
                }
            }
            String str = value.ToString();
            _FromStringValue(str, -1, -1);
            _value = _GetValue();
            _precision = -1;
            _scale = -1;
        }

        ///// <summary>
        ///// Constructor for Decoder.
        ///// </summary>
        ///// <remarks>
        ///// BsonDecimal is serialized as "type(1 byte)|name(x bytes)|size(4 bytes)|typemod(4 types)|signscale(2 bytes)|weight(2 bytes)|digits(2*digits.Length bytes)"
        ///// </remarks>
        ///// <param name="size">Total size of this decimal(4+4+2+2+digits.Length).</param>
        ///// <param name="typemod">The combined precision/scale value.
        ///// precision = (typemod >> 16) & 0xffff;scale = typemod & 0xffff;</param>
        ///// <param name="signscale">The combined sign/scale value.
        ///// sign = signscale & 0xC000;scale = signscale & 0x3FFF;</param>
        ///// <param name="weight">Weight of this decimal(NBASE=10000).</param>
        ///// <param name="digits">Real data.</param>
        public BsonDecimal(int size, int typemod, short signscale, short weight, short[] digits)
            : base(BsonType.Decimal)
        {
            // check
            if ((size < DECIMAL_HEADER_SIZE) ||
                (size == DECIMAL_HEADER_SIZE && (digits != null && digits.Length != 0)) ||
                (size > DECIMAL_HEADER_SIZE && (digits == null ||digits.Length == 0)))
            {
                var message = string.Format("Invalid arguments: {0}, {1}, {2}, {3}, {4}.",
                    size, typemod, signscale, weight, digits);
                throw new ArgumentException(message);
            }

            if (digits == null)
            {
                digits = new short[0];
            }

            _size = size;
            _typemod = typemod;
            _signscale = signscale;
            _weight = weight;
            _digits = new short[digits.Length + 1]; // we need another one for carry
            _digits[0] = (short)0;
            digits.CopyTo(_digits, 1);

            // init the middle result
            _mid_typemod = _typemod;
            _mid_ndigits = digits.Length; // here we can't use "_digits.Length", "digits.Length" is the actual count
            _mid_sign = _signscale & DECIMAL_SIGN_MASK;
            _mid_dscale = _signscale & DECIMAL_DSCALE_MASK;
            _mid_weight = _weight;
            _mid_digits = new short[_digits.Length];
            _digits.CopyTo(_mid_digits, 0);
            _mid_digits_idx = 1;
            _hasCarry = false;

            // transform to string value/precision/scale
            _value = _GetValue();
            _precision = _GetPrecision();
            _scale = _GetScale();
        }

        /// <summary>
        /// Converted this BsonDecimal to a Decimal.
        /// </summary>
        /// <returns>A Decimal.</returns>
        public decimal ToDecimal()
        {
            return Decimal.Parse(_value);
        }

        /// <summary>
        /// Converts this BsonDecimal to a Double.
        /// </summary>
        /// <returns>A Double.</returns>
        public new double ToDouble()
        {
            return Decimal.ToDouble(ToDecimal());
        }

        /// <summary>
        /// Converts this BsonDecimal to a Int64.
        /// </summary>
        /// <returns>A Int64.</returns>
        public new long ToInt64()
        {
            return Decimal.ToInt64(ToDecimal());
        }

        /// <summary>
        /// Converts this BsonDecimal to a Int32.
        /// </summary>
        /// <returns>A Int32.</returns>
        public new int ToInt32()
        {
            return Decimal.ToInt32(ToDecimal());
        }

        /// <summary>
        /// Creates a new instance of the BsonDecimal class.
        /// </summary>
        /// <param name="size">Total size of this decimal(4+4+2+2+digits.Length).</param>
        /// <param name="typemod">The combined precision/scale value.
        /// precision = (typmod >> 16) & 0xffff;scale = typmod & 0xffff;</param>
        /// <param name="signscale">The combined sign/scale value.
        /// sign = signscale & 0xC000;scale = signscale & 0x3FFF;</param>
        /// <param name="weight">Weight of this decimal(NBASE=10000).</param>
        /// <param name="digits">Real data.</param>
        /// <returns>A BsonDecimal.</returns>
        public static BsonDecimal Create(int size, int typemod, short signscale, short weight, short[] digits)
        {
            return new BsonDecimal(size, typemod, signscale, weight, digits);
        }

        // public static methods
        /// <summary>
        /// Creates a new instance of the BsonDecimal class.
        /// </summary>
        /// <param name="value">A decimal.</param>
        /// <returns>A BsonDecimal.</returns>
        public static BsonDecimal Create(decimal value)
        {
            return new BsonDecimal(value);
        }

        /// <summary>
        /// Creates a new instance of the BsonDecimal class.
        /// </summary>
        /// <param name="value">An object to be mapped to a BsonDecimal.</param>
        /// <returns>A BsonDecimal or null.</returns>
        public new static BsonDecimal Create(object value)
        {
            if (value != null)
            {
                return (BsonDecimal)BsonTypeMapper.MapToBsonValue(value, BsonType.Decimal);
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Compares this BsonDecimal to another BsonDecimal.
        /// </summary>
        /// <param name="other">The other BsonDecimal.</param>
        /// <returns>A 32-bit signed integer that indicates whether this BsonDecimal is less than, equal to, or greather than the other.</returns>
        public int CompareTo(BsonDecimal other)
        {
            if (other == null) { return 1; }
            return _CompareTo(other);
        }

        /// <summary>
        /// Compares the BsonDecimal to another BsonValue.
        /// </summary>
        /// <param name="other">The other BsonValue.</param>
        /// <returns>A 32-bit signed integer that indicates whether this BsonDecimal is less than, equal to, or greather than the other BsonValue.</returns>
        public override int CompareTo(BsonValue other)
        {
            if (other == null) { return 1; }
            // decimal
            var otherBsonDecimal = other as BsonDecimal;
            if (otherBsonDecimal != null)
            {
                return CompareTo(otherBsonDecimal);
            }
            // double
            var otherDouble = other as BsonDouble;
            if (otherDouble != null)
            {
                return CompareTo(new BsonDecimal(otherDouble.ToString()));
            }
            // int32
            var otherInt32 = other as BsonInt32;
            if (otherInt32 != null)
            {
                return CompareTo(new BsonDecimal(otherInt32.ToString()));
            }
            // int64
            var otherInt64 = other as BsonInt64;
            if (otherInt64 != null)
            {
                return CompareTo(new BsonDecimal(otherInt64.ToString()));
            }
            // the other
            return CompareTypeTo(other);
        }

        /// <summary>
        /// Compares this BsonDecimal to another BsonDecimal.
        /// </summary>
        /// <param name="rhs">The other BsonDecimal.</param>
        /// <returns>True if the two BsonDecimal values are equal.</returns>
        public bool Equals(BsonDecimal rhs)
        {
            if (object.ReferenceEquals(rhs, null) || GetType() != rhs.GetType()) { return false; }
            if (CompareTo(rhs) != 0) return false;
            return true;
        }

        /// <summary>
        /// Compares this BsonDecimal to another object.
        /// </summary>
        /// <param name="obj">The other object.</param>
        /// <returns>True if the other object is a BsonDecimal and equal to this one.</returns>
        public override bool Equals(object obj)
        {
            return Equals(obj as BsonDecimal); // works even if obj is null or of a different type
        }

        /// <summary>
        /// Gets the hash code.
        /// </summary>
        /// <returns>The hash code.</returns>
        public override int GetHashCode()
        {
            string value = _value;
            // let "2.0" or "2.00" to be "2"
            if (value.IndexOf(".") > -1)
            {
                value = System.Text.RegularExpressions.Regex.Replace(value, "0+$", "");
                value = System.Text.RegularExpressions.Regex.Replace(value, "[.]$", "");
                if (value.Length == 0) {
                    value = "0";
                }
            }
            // see Effective Java by Joshua Bloch
            int hash = 17;
            hash = 37 * hash + BsonType.GetHashCode();
            hash = 37 * hash + value.GetHashCode();
            return hash;
        }

        // public operators
        /// <summary>
        /// Converts an decimal to a BsonDecimal.
        /// </summary>
        /// <param name="value">An decimal.</param>
        /// <returns>A BsonDecimal.</returns>
        public static implicit operator BsonDecimal(decimal value)
        {
            return BsonDecimal.Create(value);
        }

        /// <summary>
        /// Compares two BsonDecimal values.
        /// </summary>
        /// <param name="lhs">The first BsonObjectId.</param>
        /// <param name="rhs">The other BsonObjectId.</param>
        /// <returns>True if the two BsonDecimal values are not equal according to ==.</returns>
        public static bool operator !=(BsonDecimal lhs, BsonDecimal rhs)
        {
            return !(lhs == rhs);
        }

        /// <summary>
        /// Compares two BsonDecimal values.
        /// </summary>
        /// <param name="lhs">The first BsonDecimal.</param>
        /// <param name="rhs">The other BsonDecimal.</param>
        /// <returns>True if the two BsonDecimal values are equal according to ==.</returns>
        public static bool operator ==(BsonDecimal lhs, BsonDecimal rhs)
        {
            if (object.ReferenceEquals(lhs, null)) { return object.ReferenceEquals(rhs, null); }
            return lhs.Equals(rhs);
        }

        /// <summary>
        /// Returns a string representation of the value.
        /// </summary>
        /// <returns>A string representation of the value.</returns>
        public override string ToString()
        {
            //if (_precision != -1 && _scale != -1)
            //{
            //    return "{" + "\"$decimal\" : \"" + _value + "\", \"$precision\" : [ " + _precision
            //            + ", " + _scale + " ]" + "}";
            //}
            //else
            //{
            //    return "{" + "\"$decimal\" : \"" + _value + "\"}"; 
            //}

            return this.ToJson();
        }


        /**************************************************************** private method ****************************************************************/

        private void _Init(String value, int precision, int scale)
        {
            if (value == null || value == "")
            {
                throw new ArgumentException("The string of decimal value is null or empty");
            }
		    if ((precision < 0 && precision != -1) || (scale < 0 && scale != -1) || 
                (precision == -1 && scale != -1) || (scale == -1 && precision != -1 ))
            {
                var message = string.Format("Invalid precision({0}) and scale({1}).", precision, scale);
                throw new ArgumentException(message);
		    }
            if (precision == 0 || precision > DECIMAL_MAX_PRECISION || scale > precision) 
            {
                var message = string.Format("Invalid precision({0}) and scale({1}).", precision, scale);
                throw new ArgumentException(message);
		    }

            // init(never remove this, if we don't init _value, it's null)
            _value = value;
            _precision = precision;
            _scale = scale;

            // fields for helping to build decimal(result in middle)
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

            // fields for building bson
            _size = -1;
            _typemod = -1;
            _signscale = 0;
            _weight = 0;
            _digits = null;
        }

        private void _FromStringValue(String value, int precision, int scale)
        {
		    bool have_dp = false;
		    int sign = SDB_DECIMAL_POS;
		    int dweight = -1;
		    int ddigits = 0;
		    int dscale = 0;
		    int weight = -1;
		    int ndigits = 0;
		    int offset = 0;

            _Init(value, precision, scale);

            char[] cp = _value.ToCharArray();
            int cp_idx = 0;
            if (_value.Length >= 3)
            {
			    // not a number
			    if ((cp[0] == 'n' || cp[0] == 'N') && 
                    (cp[1] == 'a' || cp[1] == 'A') && 
                    (cp[2] == 'n' || cp[2] == 'N')) 
                {
				    _SetNan();
                    _Update();
				    return;
			    }
			    // min
			    if ((cp[0] == 'm' || cp[0] == 'M') && 
                    (cp[1] == 'i' || cp[1] == 'I') && 
                    (cp[2] == 'n' || cp[2] == 'N')) 
                {
				    _SetMin();
                    _Update();
				    return;
			    }
			    // max
			    if ((cp[0] == 'm' || cp[0] == 'M') && 
                    (cp[1] == 'a' || cp[1] == 'A') && 
                    (cp[2] == 'x' || cp[2] == 'X')) 
                {
				    _SetMax();
                    _Update();
				    return;
			    }
            }

		    /*
		     * We first parse the string to extract decimal digits and determine the
		     * correct decimal weight. Then convert to NBASE representation.
		     */
		    switch (cp[cp_idx]) 
            {
		    case '+':
			    sign = SDB_DECIMAL_POS;
			    cp_idx++;
			    break;

		    case '-':
			    sign = SDB_DECIMAL_NEG;
			    cp_idx++;
			    break;
		    }

		    if (cp[cp_idx] == '.') 
            {
			    have_dp = true;
			    cp_idx++;
		    }

            // the first digit should be a number
            if (!Char.IsDigit(cp[cp_idx]))
            {
                var message = string.Format("Invalid decimal: {0}.", _value);
                throw new ArgumentException(message);
            }

		    // actually, "decdigitsnum" is more than what we need, we need to make
		    // sure
		    // we have enough space for holding the digits
		    int decdigits_idx = 0;
		    int decdigitsnum = (cp.Length - cp_idx) + DECIMAL_DEC_DIGITS * 2;
		    int[] decdigits = new int[decdigitsnum];
		    decdigits_idx = DECIMAL_DEC_DIGITS;

		    while (cp_idx < cp.Length) 
            {
			    if (Char.IsDigit(cp[cp_idx])) 
                {
                    // return "-1.0" for failure
                    double ret = Char.GetNumericValue(cp[cp_idx++]);
                    if (ret < -0.9 && ret > -1.1)
                    {
                        var message = string.Format("Invalid decimal value: {0}.", _value);
                        throw new ArgumentException(message);
                    }
                    int d = (int)ret;
				    decdigits[decdigits_idx++] = d;
				    if (!have_dp) 
                    {
					    dweight++;
                        if (dweight >= DECIMAL_MAX_DWEIGHT + DECIMAL_MAX_PRECISION)
                        {
                            var message = string.Format("The integer part of the value is out of bound.");
                            throw new ArgumentException(message);
                        }
				    } 
                    else 
                    {
					    dscale++;
                        if (dscale > DECIMAL_MAX_DSCALE + DECIMAL_MAX_PRECISION)
                        {
                            var message = string.Format("The decimal part of the value is out of bound.");
                            throw new ArgumentException(message);
                        }
				    }
			    } 
                else if (cp[cp_idx] == '.') 
                {
				    if (have_dp) 
                    {
                        var message = string.Format("Invalid decimal point in decimal: {0}.", _value);
                        throw new ArgumentException(message);
				    } 
                    else 
                    {
					    have_dp = true;
					    cp_idx++;
				    }
			    } 
                else 
                {
				    if (cp[cp_idx] != 'e' && cp[cp_idx] != 'E') 
                    {
					    // invalid argument
                        var message = string.Format("Invalid digit in decimal: {0}.", _value);
                        throw new ArgumentException(message);
				    } 
                    else 
                    {
					    // when cp[cp_idx] is 'e' or 'E', let's handle it later
					    break;
				    }
			    }
		    }

		    // "ddigits" does not contain '.'
		    ddigits = decdigits_idx - DECIMAL_DEC_DIGITS;

		    /* Handle exponent, if any */
		    if ((cp_idx < cp.Length) && (cp[cp_idx] == 'e' || cp[cp_idx] == 'E')) 
            {
			    long exponent = 0L;
			    cp_idx++;
                // check the charator of the digits in exponent
                int tmp_cp_idx = cp_idx;
                if ((tmp_cp_idx < cp.Length) && (cp[tmp_cp_idx] == '+' || cp[tmp_cp_idx] == '-'))
                {
                    tmp_cp_idx++;
                }
                for (; tmp_cp_idx < cp.Length; tmp_cp_idx++)
                {
                    if (!Char.IsDigit(cp[tmp_cp_idx]))
                    {
                        var message = string.Format("Invalid exponent in decimal value: {0}", _value);
                        throw new ArgumentException(message);
                    }
                }
                // try to transform chars to exponent
                StringBuilder strBuilder = new StringBuilder();
                strBuilder.Append(cp, cp_idx, cp.Length - cp_idx);
			    String strexponent = strBuilder.ToString();
                if ( strexponent == null )
                {
                    var message = string.Format("Failed to parse exponent in decimal value: {0}", _value);
                    throw new ArgumentException(message);
                }
			    try 
                {
				    exponent = Convert.ToInt64(strexponent);
			    } 
                catch (FormatException e) 
                {
                    var message = string.Format("Invalid exponent in decimal value: {0}", _value);
                    throw new ArgumentException(message);
                }
                catch (OverflowException e)
                {
                    var message = string.Format("Invalid exponent in decimal value: {0}", _value);
                    throw new ArgumentException(message);
			    }
                // check the exponent
			    if (exponent > DECIMAL_MAX_PRECISION || exponent < -DECIMAL_MAX_PRECISION) 
                {
                    var message = string.Format("Exponent in decimal value: {0} should " +
                    "be in the bound of [-1000, 1000]", _value);
                    throw new ArgumentException(message);
			    }
                // adjust
			    dweight += (int) exponent;
			    dscale -= (int) exponent;
			    if (dscale < 0) 
                {
				    dscale = 0;
			    }
		    }

            // make sure the count of digits is in the bound
            if (dweight >= DECIMAL_MAX_DWEIGHT)
            {
                var message = string.Format("The integer part of the value is out of bound.");
                throw new ArgumentException(message);
            }
            if (dscale > DECIMAL_MAX_DSCALE)
            {
                var message = string.Format("The decimal part of the value is out of bound.");
                throw new ArgumentException(message);
            }

		    /*
		     * Okay, convert pure-decimal representation to base NBASE. First we
		     * need to determine the converted weight and ndigits. offset is the
		     * number of decimal zeroes to insert before the first given digit to
		     * have a correctly aligned first NBASE digit.
		     */
		    if (dweight >= 0) 
            {
			    // "dweight + 1" is the count of digits in the left of decimal
			    // point,
			    // "(dweight + 1 + DECIMAL_DEC_DIGITS - 1) / DECIMAL_DEC_DIGITS"
			    // tells us which part is the first digit in,
			    // because when dweight >= 0, weight starts from 0(the first part in
			    // the left of decimal point), so we need to decrease 1
			    weight = (dweight + 1 + DECIMAL_DEC_DIGITS - 1) / DECIMAL_DEC_DIGITS - 1;
		    } 
            else 
            {
			    // "-dweight - 1" means how many "0" exists ahead the first valid
			    // digit,
			    // for example, "0.00000123", "dweight" is -6, "-dweight - 1" means
			    // there
			    // has 5 "0" ahead the first valid digit "1".
			    // "(-dweight - 1)/DECIMAL_DEC_DIGITS + 1" tells us which part is
			    // the
			    // first valid digit in, because when dweight < 0, weight starts
			    // from -1(
			    // the first part in the right of decimal point), so we need to
			    // increase 1
			    weight = -((-dweight - 1) / DECIMAL_DEC_DIGITS + 1);
		    }
		    // notic that: when we input a decimal in format of ".123456" or
		    // ".01234e5"
		    // "dweight","weight" and "offset" do not express their own meaning
		    // accurately, but function "_Strip" will help us to fix the
		    // "weight", and what we need to care about is the corrent "weight".

		    offset = (weight + 1) * DECIMAL_DEC_DIGITS - (dweight + 1);
		    ndigits = (ddigits + offset + DECIMAL_DEC_DIGITS - 1)
				    / DECIMAL_DEC_DIGITS;

		    // alloc buffer for keeping digits
		    _Alloc(ndigits);

		    // keep result to local
		    _mid_sign = sign;
		    _mid_weight = weight;
		    _mid_dscale = dscale;

		    decdigits_idx = DECIMAL_DEC_DIGITS - offset;
		    while (ndigits-- > 0) 
            {
			    _mid_digits[_mid_digits_idx++] = (short) (((decdigits[decdigits_idx] * 10 + decdigits[decdigits_idx + 1]) * 10 + decdigits[decdigits_idx + 2]) * 10 + decdigits[decdigits_idx + 3]);
			    decdigits_idx += DECIMAL_DEC_DIGITS;
		    }

		    /* Strip any leading/trailing zeroes, and normalize weight if zero */
		    _Strip();

		    _ApplyTypeMod();

            _Update();
        }

        private int _GetExpectCharSize()
        {
		    int retSize = 0;
		    if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN && 
                _mid_dscale == SDB_DECIMAL_SPECIAL_NAN) 
            {
			    return (3); // "NAN"
		    }

		    if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN && 
                _mid_dscale == SDB_DECIMAL_SPECIAL_MIN) 
            {
			    return (3); // "MIN"
		    }

		    if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN && 
                _mid_dscale == SDB_DECIMAL_SPECIAL_MAX) 
            {
			    return (3); // "MAX"
		    }

            /*
             * Allocate space for the result.
             * 
             * tmpSize is set to the number of decimal digits before decimal point.
             * _mid_dscale is the number of decimal digits we will print after decimal point.
             * We may generate as many as DEC_DIGITS-1 excess digits at the end, and
             * in addition we need room for sign, decimal point, null terminator.
             */
            retSize = (_mid_weight + 1) * DECIMAL_DEC_DIGITS;
		    if (retSize <= 0) 
            {
			    retSize = 1;
		    }

		    retSize += _mid_dscale + DECIMAL_DEC_DIGITS + 2;

		    return retSize;
        }


	    private bool _IsNan() 
        {
		    if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN && 
                _mid_dscale == SDB_DECIMAL_SPECIAL_NAN) 
            {
			    return true;
		    }
		    return false;
	    }

	    private bool _IsMin() {
		    if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN && 
                _mid_dscale == SDB_DECIMAL_SPECIAL_MIN) 
            {
			    return true;
		    }
		    return false;
	    }

	    private bool _IsMax() {
		    if (_mid_sign == SDB_DECIMAL_SPECIAL_SIGN && 
                _mid_dscale == SDB_DECIMAL_SPECIAL_MAX) 
            {
			    return true;
		    }
		    return false;
	    }

        private String _GetValue()
        {
		    short dig = 0;
		    short d1 = 0;
		    int d = 0;
            String retStr = null;

		    int expect_char_size = _GetExpectCharSize();
		    char[] cp = new char[expect_char_size];
		    int cp_idx = 0;
		    int cp_idx_end = 0;
		    // decimal is nan
		    if (_IsNan()) 
            {
			    cp[0] = 'N';
			    cp[1] = 'a';
			    cp[2] = 'N';
                retStr = new String(cp);
                return retStr;
		    }
		    // decimal is min
		    if (_IsMin()) 
            {
			    cp[0] = 'M';
			    cp[1] = 'I';
			    cp[2] = 'N';
                retStr = new String(cp);
                return retStr;
		    }
		    // decimal is max
		    if (_IsMax()) 
            {
			    cp[0] = 'M';
			    cp[1] = 'A';
			    cp[2] = 'X';
                retStr = new String(cp);
                return retStr;
		    }
		    // output a dash for negative values
		    if (_mid_sign == SDB_DECIMAL_NEG) 
            {
			    cp[cp_idx++] = '-';
		    }
		    // output all digits before the decimal point
		    if (_mid_weight < 0) 
            {
			    d = _mid_weight + 1;
			    cp[cp_idx++] = '0';
		    } 
            else 
            {
			    for (d = 0; d <= _mid_weight; d++) 
                {
				    // for _digits[0] is placed carry, so we need to known whether these has carry or not.
                    // if so, we start from position 0, otherwise, we start from position 1
                    if (_hasCarry)
                    {
                        dig = ((d < _mid_ndigits) ? _mid_digits[d] : (short)0);
                    }
                    else
                    {
                        dig = ((d < _mid_ndigits) ? _mid_digits[1 + d] : (short)0);
                    }
				    
				    // in the first digit, suppress extra leading decimal zeroes
				    {
					    bool putit = (d > 0);

					    d1 = (short) (dig / 1000);
					    dig -= (short)(d1 * 1000);
					    putit |= (d1 > 0);
					    if (putit) 
                        {
						    cp[cp_idx++] = (char) (d1 + '0');
					    }

					    d1 = (short) (dig / 100);
					    dig -= (short)(d1 * 100);
					    putit |= (d1 > 0);
					    if (putit) 
                        {
						    cp[cp_idx++] = (char) (d1 + '0');
					    }

					    d1 = (short) (dig / 10);
					    dig -= (short)(d1 * 10);
					    putit |= (d1 > 0);
					    if (putit) 
                        {
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
			    for (int i = 0; i < _mid_dscale; d++, i += DECIMAL_DEC_DIGITS) 
                {
                    // for _digits[0] is placed carry, so we need to known whether these has carry or not.
                    // if so, we start from position 0, otherwise, we start from position 1
                    if (_hasCarry)
                    {
                        dig = (d >= 0 && d < _mid_ndigits) ? _mid_digits[d] : (short)0;
                    }
                    else
                    {
                        dig = (d >= 0 && d < _mid_ndigits) ? _mid_digits[1 + d] : (short)0;
                    }
				    
				    d1 = (short)(dig / 1000);
				    dig -= (short)(d1 * 1000);
				    cp[cp_idx++] = (char)(d1 + '0');
				
				    d1 = (short)(dig / 100);
				    dig -= (short)(d1 * 100);
				    cp[cp_idx++] = (char)(d1 + '0');
				
				    d1 = (short)(dig / 10);
				    dig -= (short)(d1 * 10);
				    cp[cp_idx++] = (char)(d1 + '0');
				
				    cp[cp_idx++] = (char)(dig + '0');
			    }
			    // we need to add some zeros to tail sometimes
			    while(cp_idx < cp_idx_end) 
                {
				    cp[cp_idx++] = '0';
			    }
			    // we don't need the extra zeros in the tail
			    while(cp_idx_end < cp_idx) 
                {
				    cp[cp_idx_end] = (char)(cp[cp_idx_end] - '0');
				    cp_idx_end++;
			    }
		    }
		    // build the return BSONDecimal
            retStr = new String(cp).Trim();
            retStr = retStr.Remove(retStr.IndexOf("\0"));
            return retStr;
        }

	    private int _GetPrecision() 
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

	    private int _GetScale() 
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

	    private void _Alloc(int ndigits) 
        {
		    // we need another short for carry,
		    // when rounding happen, we may use this short
		    _mid_digits = new short[ndigits + 1]; 
		    _mid_digits_idx = 0;
		    _mid_digits[_mid_digits_idx++] = 0;
		    _mid_ndigits = ndigits;
	    }

	    private void _Strip() 
        {
		    int ndigits = _mid_ndigits;
		    int digits_idx_f = 1;
		    // before _Round(), _digits starts from index 1, so we need to add 1
		    int digits_idx_e = (ndigits + 1) - 1;

		    // strip leading zeroes
		    while (ndigits > 0 && _mid_digits[digits_idx_f] == 0) 
            {
			    digits_idx_f++;
			    _mid_weight--;
			    ndigits--;
		    }

		    // strip trailing zeroes
		    while (ndigits > 0 && _mid_digits[digits_idx_e] == 0) 
            {
			    digits_idx_e--;
			    ndigits--;
		    }

		    int count = digits_idx_e - digits_idx_f + 1;
		    if (count != ndigits) 
            {
                var message = string.Format("ndigits[{0}] is not equal with [{1}]", ndigits, count);
                // TODO: we need to throw another expection
                throw new ArgumentException(message);
		    }

		    // if it's zero, normalize the sign and weight
		    if (ndigits == 0) 
            {
			    _mid_sign = SDB_DECIMAL_POS;
			    _mid_weight = 0;
		    }

		    // update the local results, when the begin index is not in position 1
		    if (digits_idx_f != 1) 
            {
			    for (int i = digits_idx_f, begin_idx = 1; i <= digits_idx_e; ++i, ++begin_idx) 
                {
				    _mid_digits[begin_idx] = _mid_digits[i];
				    _mid_digits[i] = 0;
			    }
		    }
		    for (int i = digits_idx_e + 1; i < _mid_digits.Length; ++i) 
            {
			    _mid_digits[i] = 0;
		    }
            // reset it to the beginning
		    _mid_digits_idx = 1;
		    _mid_ndigits = ndigits;
	    }

	    private void _ApplyTypeMod() 
        {
		    int precision = 0;
		    int scale = 0;
		    int maxdigits = 0;
		    int ddigits = 0;
		    int i = 0;

		    // Do nothing if we have a default _typemod(-1)
		    if (_mid_typemod == -1) 
            {
			    return;
		    }

		    precision = (_mid_typemod >> 16) & 0xffff;
		    scale = _mid_typemod & 0xffff;
		    maxdigits = precision - scale;

		    // Round to target scale (and set _dscale)
		    _Round(scale);

		    /*
		     * Check for overflow - note we can't do this before rounding, because
		     * rounding could raise the weight. Also note that the var's weight
		     * could be inflated by leading zeroes, which will be stripped before
		     * storage but perhaps might not have been yet. In any case, we must
		     * recognize a true zero, whose weight doesn't mean anything.
		     */
		    ddigits = (_mid_weight + 1) * DECIMAL_DEC_DIGITS;
		    if (ddigits > maxdigits) 
            {
			    // Determine true weight; and check for all-zero result
                if (!_hasCarry)
                {
                    i = 1;
                }
                else
                {
                    i = 0;
                }
			    for (; i <= _mid_ndigits; i++) 
                {
				    short dig = _mid_digits[i];
				    if (dig != 0) 
                    {
					    // Adjust for any high-order decimal zero digits
					    if (dig < 10) 
                        {
						    ddigits -= 3;
					    } 
                        else if (dig < 100) 
                        {
						    ddigits -= 2;
					    } 
                        else if (dig < 1000) 
                        {
						    ddigits -= 1;
					    }

					    if (ddigits > maxdigits) 
                        {
                            var message = string.Format("the input decimal[{0}] can't be " + 
								    "expressed in the range which is delimited by " +
								    "the given precision[{1}] and scale[{2}]", _value, precision, scale);
                            throw new ArgumentException(message);
					    }
					    break;
				    }
				    ddigits -= DECIMAL_DEC_DIGITS;
			    }
		    }
	    }

	    private void _Round(int rscale) 
        {
		    int di = 0;
		    int ndigits = 0;
		    int carry = 0;

		    _mid_dscale = rscale;

		    // decimal digits wanted
		    di = (_mid_weight + 1) * DECIMAL_DEC_DIGITS + rscale;

		    /*
		     * If di = 0, the value loses all digits, but could round up to 1 if its
		     * first extra digit is >= 5. If di < 0 the result must be 0.
		     */
		    if (di < 0) 
            { 
			    _mid_ndigits = 0;
			    _mid_weight = 0;
			    _mid_sign = SDB_DECIMAL_POS;
		    } 
            else 
            {
			    // NBASE digits wanted
			    ndigits = (di + DECIMAL_DEC_DIGITS - 1) / DECIMAL_DEC_DIGITS;

			    // 0, or number of decimal digits to keep in last NBASE digit
			    di %= DECIMAL_DEC_DIGITS;

			    if ((ndigits < _mid_ndigits) || (ndigits == _mid_ndigits && di > 0)) 
                {
				    _mid_ndigits = ndigits;
				    if (di == 0) 
                    {
					    // for digits in _digits starts from position 1 but not 0,
					    // so we need to increase 1
					    carry = (_mid_digits[ndigits + 1] >= DECIMAL_HALF_NBASE) ? 1
							    : 0;
				    } 
                    else 
                    {
					    // Must round within last NBASE digit
					    int extra = 0;
					    int pow10 = 0;
					    pow10 = round_powers[di];

					    extra = _mid_digits[--ndigits + 1] % pow10;
					    _mid_digits[ndigits + 1] -= (short)extra;
					    carry = 0;
					    if (extra >= pow10 / 2) 
                        {
						    pow10 += _mid_digits[ndigits + 1];
						    if (pow10 >= DECIMAL_NBASE) 
                            {
							    pow10 -= DECIMAL_NBASE;
							    carry = 1;
						    }
						    _mid_digits[ndigits + 1] = (short) pow10;
					    }
				    }

				    // Propagate carry if needed
				    while (carry != 0) 
                    {
					    carry += _mid_digits[--ndigits + 1];
					    if (carry >= DECIMAL_NBASE) 
                        {
						    _mid_digits[ndigits + 1] = (short) (carry - DECIMAL_NBASE);
						    carry = 1;
					    } 
                        else 
                        {
						    _mid_digits[ndigits + 1] = (short) carry;
						    carry = 0;
					    }
				    }

				    if (ndigits < 0) 
                    {
					    // better not have added > 1 digit
					    if (ndigits != -1) 
                        {
                            // TODO:
						    throw new ArgumentException("ndigits[" + ndigits
								    + "] is not -1");
					    }
					    if (_hasCarry) 
                        {
                            // TODO:
						    throw new ArgumentException(
								    "impossible for _hasCarry to be true");
					    }
					    if (_mid_digits[0] == 0) 
                        {
                            // TODO:
						    throw new ArgumentException(
								    "impossible for _mid_digits[0] to be 0");
					    }
					    _hasCarry = true;
					    _mid_ndigits++;
					    _mid_weight++;
				    }
			    }
		    }
	    }

        private void _Update()
        {
            // 1. _size
            _size = DECIMAL_HEADER_SIZE + _mid_ndigits * sizeof(short);
            // 2. _typemod
            _typemod = _IsSpecial(this) ? -1 : _mid_typemod;
            // 3. _signscale
            _signscale = (short)((_mid_dscale & DECIMAL_DSCALE_MASK) | _mid_sign);
            // 4. _weight
            _weight = (short)_mid_weight;
            // 5. _digits
            if (_mid_digits == null || _mid_digits.Length == 0 || _mid_ndigits == 0)
            {
                _digits = _mid_digits = new short[0];
            }
            else
            {
               _digits = _mid_digits;
            }
        }

        private int _CompareTo(BsonDecimal other)
        {
            if (other == null) { return 1; }

            // min is equal min;  min is less than any non-min.
            if (_IsMin(this))
            {
                if (_IsMin(other))
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else if (_IsMin(other))
            {
                return 1;
            }

            // max is equal max;  max is larger than any non-max.
            if (_IsMax(this))
            {
                if (_IsMax(other))
                {
                    return 0;
                }
                else
                {
                    return 1;
                }
            }
            else if (_IsMax(other))
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
            if (_IsNan(this))
            {
                if (_IsNan(other))
                {
                    return 0; /* NAN = NAN */
                }
                else
                {
                    return -1; /* NAN < non-NAN */
                }
            }
            else if (_IsNan(other))
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

                return _CompareABS(other);
            }

            if (other._mid_sign == SDB_DECIMAL_POS)
            {
                return -1;
            }
            // note that, when we come here, we both "this" and "other" are negative number
            // so, we need to return the opposite result
            return -_CompareABS(other);
        }

        private bool _IsMin(BsonDecimal value)
        {
            if (value == null)
            {
                return false;
            }
            if (_IsSpecial(value) && value._mid_dscale == SDB_DECIMAL_SPECIAL_MIN)
            {
                return true;
            }
            return false;
        }

        private bool _IsMax(BsonDecimal value)
        {
            if (value == null)
            {
                return false;
            }
            if (_IsSpecial(value) && value._mid_dscale == SDB_DECIMAL_SPECIAL_MAX)
            {
                return true;
            }
            return false;
        }

        private bool _IsNan(BsonDecimal value)
        {
            if (value == null)
            {
                return true;
            }
            if (_IsSpecial(value) && value._mid_dscale == SDB_DECIMAL_SPECIAL_NAN)
            {
                return true;
            }
            return false;
        }

        private bool _IsSpecial(BsonDecimal value)
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

	    private void _SetNan() 
        {
		    _mid_ndigits = 0;
		    _mid_weight = 0;
		    _mid_sign = SDB_DECIMAL_SPECIAL_SIGN;
		    _mid_dscale = SDB_DECIMAL_SPECIAL_NAN;
	    }

	    private void _SetMin() 
        {
		    _mid_ndigits = 0;
		    _mid_weight = 0;
		    _mid_sign = SDB_DECIMAL_SPECIAL_SIGN;
		    _mid_dscale = SDB_DECIMAL_SPECIAL_MIN;
	    }

	    private void _SetMax() 
        {
		    _mid_ndigits = 0;
		    _mid_weight = 0;
		    _mid_sign = SDB_DECIMAL_SPECIAL_SIGN;
		    _mid_dscale = SDB_DECIMAL_SPECIAL_MAX;
	    }

        private int _CompareABS(BsonDecimal other)
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

            // we use "xxx.Digits", so we do not need to care about carry
            digits1 = this.Digits;
            digits2 = other.Digits;            

            // check whether digits1 and digits2 will be null or not
            if (digits1 == null || digits2 == null)
            {
                var message = "";
                if (digits1 == null)
                {
                    message = string.Format("Failed to compare for no digits in current decimal.");
                }
                else
                {
                    message = string.Format("Failed to compare for no digits in other decimal.");
                }
                throw new InvalidOperationException(message);
            }

            weight1 = this.Weight;
            ndigit1 = digits1.Length;

            weight2 = other.Weight;
            ndigit2 = digits2.Length;

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
}
