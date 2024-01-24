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

    /** \class BsonDefaults
     *  \brief A static helper class containing BSON defaults.
     */
    public static class BsonDefaults
    {
        // private static fields
        private static GuidRepresentation __guidRepresentation = GuidRepresentation.CSharpLegacy;
        private static int __maxDocumentSize = 16 * 1024 * 1024 + 16 * 1024; // 16MiB + 16K
        private static int __maxSerializationDepth = 100;
        private static bool __compatible = false;

        // public static properties
        /// <summary>
        /// Gets or sets the default representation to be used in serialization of 
        /// Guids to the database. 
        /// <seealso cref="SequoiaDB.Bson.GuidRepresentation"/> 
        /// </summary>
        public static GuidRepresentation GuidRepresentation
        {
            get { return __guidRepresentation; }
            set { __guidRepresentation = value; }
        }

        /// <summary>
        /// Gets or sets the default max document size. The default is 16MiB.
        /// </summary>
        public static int MaxDocumentSize
        {
            get { return __maxDocumentSize; }
            set { __maxDocumentSize = value; }
        }

        /// <summary>
        /// Gets or sets the default max serialization depth (used to detect circular references during serialization). The default is 100.
        /// </summary>
        public static int MaxSerializationDepth
        {
            get { return __maxSerializationDepth; }
            set { __maxSerializationDepth = value; }
        }

        /// <summary>
        /// When "compatible" is true, the content of BsonDocument method "toString" is show 
        ///       absolutely the same with which is show in sdb shell.
        /// compatible true or false, default to be false;
        ///
        /// {@code
        /// // we have a bson as below:
        ///  BSONObject obj = new BasicBSONObject("a", Long.MAX_VALUE);
        /// // sdb shell shows this bson like this:
        /// {"a" : { "$numberLong" : "9223372036854775807"}}
        /// // sdb shell use javascript grammer, so, it can't display number
        /// // which is great that 2^53 - 1. So it use "$numberLong" to represent
        /// // the type, and keep the number between the quotes.
        /// // However, in java, when we use "obj.toString()", 
        /// // most of the time, we don't hope to get a result with 
        /// // the format "$numberLong", we hope to see the result as 
        /// // below:
        /// {"a" : 9223372036854775807}
        /// // When parameter "compatible" is false, we get this kind of result
        /// // all the time. Otherwise, we get a result which is show as the sdb shell shows.
        /// }
        /// </summary>
        public static bool JsCompatibility
        {
            get { return __compatible; }
            set { __compatible = value; }
        }

    }
}
