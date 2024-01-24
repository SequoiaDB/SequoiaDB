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
using System.Collections.Generic;
using System.Linq;
using System.Text;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Zhaobo Tan
 */
namespace SequoiaDB
{
    /** \class ClientOptions
     *  \brief Client Configuration Options
     */
    public class ClientOptions
    {
        // milliseconds
        private long cacheInterval = 300000;
        private bool enableCache = true;

        /** \property CacheInterval
         *  \brief Get or set the caching interval(milliseconds)
         */
        public long CacheInterval
        {
            get
            {
                return cacheInterval;
            }
            set
            {
                cacheInterval = value;
            }
        }

        /** \property EnableCache
         *  \brief Get or set caching the name of collection space and collection in client or not.
         */
        public bool EnableCache
        {
            get
            {
                return enableCache;
            }
            set
            {
                enableCache = value;
            }
        }

    }
}
