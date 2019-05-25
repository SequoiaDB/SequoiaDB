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
