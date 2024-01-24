/*
 * Copyright 2018 SequoiaDB Inc.
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
using SequoiaDB.Bson;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class BaseException
     *  \brief Database operation exception
     */
    public class BaseException : Exception
    {
        private const long serialVersionUID = -6115487863398926195L;

        private string message;
        private string errorType;
        private int errorCode;
        private BsonDocument errorObject;

        /// <summary>
        /// Expection throw by sequoiadb.
        /// </summary>
        /// <param name="errorCode">The error code return by engine</param>
        /// <param name="detail">The error Detail</param>
        /// <param name="errorObject">The error object return from engine</param>
        internal BaseException(int errorCode, String detail, BsonDocument errorObject)
        {
            try
            {
                this.errorObject = errorObject;
                if (detail != null && detail != "")
                {
                    this.message = SDBErrorLookup.GetErrorDescriptionByCode(errorCode) +
                        ", " + detail;
                }
                else
                {
                    this.message = SDBErrorLookup.GetErrorDescriptionByCode(errorCode);
                }
                this.errorType = SDBErrorLookup.GetErrorTypeByCode(errorCode);
                this.errorCode = errorCode;
            }
            catch (Exception)
            {
                this.message = SequoiadbConstants.UNKONWN_DESC;
                this.errorType = SequoiadbConstants.UNKNOWN_TYPE;
                this.errorCode = SequoiadbConstants.UNKNOWN_CODE;
            }
        }

        /// <summary>
        /// Expection throw by sequoiadb.
        /// </summary>
        /// <param name="errorCode">The error code return by engine</param>
        /// <param name="detail">The error Detail</param>
        internal BaseException(int errorCode, string detail)
            : this(errorCode, detail, null)
        {
        }

        /// <summary>
        /// Expection throw by sequoiadb.
        /// </summary>
        /// <param name="errorCode">The error code return by engine</param>
        /// <param name="errorObject">The error object return from engine</param>
        internal BaseException(int errorCode, BsonDocument errorObject)
            : this(errorCode, "", errorObject)
        {
        }

        /// <summary>
        /// Expection throw by sequoiadb.
        /// </summary>
        /// <param name="errorCode">The error code to throw</param>
        internal BaseException(int errorCode)
            : this(errorCode, "")
        {
        }

        /// <summary>
        /// Expection throw by sequoiadb.
        /// </summary>
        /// <param name="errorType">The error type to throw</param>
        internal BaseException(string errorType)
        {
            try
            {
                this.message = SDBErrorLookup.GetErrorDescriptionByType(errorType);
                this.errorCode = SDBErrorLookup.GetErrorCodeByType(errorType);
                this.errorType = errorType;
            }
            catch (Exception)
            {
                this.message = SequoiadbConstants.UNKONWN_DESC;
                this.errorType = SequoiadbConstants.UNKNOWN_TYPE;
                this.errorCode = SequoiadbConstants.UNKNOWN_CODE;
            }
        }

        /** \property Message
         *  \brief Get the error description of exception
         */
        public override string Message
        {
            get
            {
                return this.message;
            }
        }

        /** \property ErrorType
         *  \brief Get the error type of exception
         */
        public string ErrorType
        {
            get 
            {
                return this.errorType;
            }
        }

        /** \property ErrorCode
         *  \brief Get the error code of exception
         */
        public int ErrorCode
        {
            get
            {
                return this.errorCode;
            }
        }

        /** \property ErrorObject
         *  \brief Get the error object. When database try to tell the user what error happen in engine,
         *         it will  merge all the error information, and return it by an bson.
         *  \return The error object got from engine or null for no error object got from engine.
         *          If there has an error, it contains the follow fields:
         *          <ul>
         *              <li>errno:       the error number.</li>
         *              <li>description: the description of the errno.</li>
         *              <li>detail:      the error detail.</li>
         *              <li>
         *                  ErrNodes:    describes which data nodes have errors, and detailed information about the error
         *                  (this field is an expand field, which is only returned when an error occurs on the data node).
         *              </li>
         *          </ul>

         */
        public BsonDocument ErrorObject
        {
            get
            {
                return this.errorObject;
            }
        }
    }
}
