/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class ConfigOptions
     *  \brief Database Connection Configuration Options
     */
    public class ConfigOptions
    {
        // seconds
        private int maxAutoConnectRetryTime = 15;
        // milliseconds
        private int connectTimeout = 10000;
        // Send and Reive Timeout: default is no timeout
        private int sendTimeout = 0;
        private int receiveTimeout = 0;
        private int keepIdle = 15000;//15s
        private int keepInterval = 3000;//3s

        private bool socketKeepAlive = false;
        private bool useNagle = false;
        private bool useSSL = false;

        /** \property MaxAutoConnectRetryTime
         *  \brief Get or group the max autoconnect retry time(seconds)
         */
        public int MaxAutoConnectRetryTime
        {
            get
            {
                return maxAutoConnectRetryTime;
            }
            set
            {
                maxAutoConnectRetryTime = value;
            }
        }

        /** \property ConnectTimeout
         *  \brief Get or group the connect timeout(milliseconds)
         */
        public int ConnectTimeout
        {
            get
            {
                return connectTimeout;
            }
            set
            {
                connectTimeout = value;
            }
        }

        /** \property SendTimeout
         *  \brief Get or group the send timeout(milliseconds)
         */
        public int SendTimeout
        {
            get
            {
                return sendTimeout;
            }
            set
            {
                sendTimeout = value;
            }
        }

        /** \property ReceiveTimeout
         *  \brief Get or group the receive timeout(milliseconds)
         */
        public int ReceiveTimeout
        {
            get
            {
                return receiveTimeout;
            }
            set
            {
                receiveTimeout = value;
            }
        }

        /** \property KeepIdle
         *  \brief Get or set keep alive idle(milliseconds)
         */
        public int KeepIdle
        {
            get
            {
                return keepIdle;
            }
            set
            {
                keepIdle = value;
            }
        }

        /** \property KeepInterval
         *  \brief Get or set keep alive interval(milliseconds)
         */
        public int KeepInterval
        {
            get
            {
                return keepInterval;
            }
            set
            {
                keepInterval = value;
            }
        }

        /** \property UseNagle
         *  \brief Get or group the Nagle Algorithm
         */
        public bool UseNagle
        {
            get
            {
                return useNagle;
            }
            set
            {
                useNagle = value;
            }
        }

        /** \property UseKeepAlive
         *  \brief Get or set whether use the keep alive or not, default to be open,
         *  and the time is 15s, the interval is 3s
         */
        public bool SocketKeepAlive
        {
            get
            {
                return socketKeepAlive;
            }
            set
            {
                socketKeepAlive = value;
            }
        }

        /** \property UseSSL
         *  \brief Get or set whether use the SSL or not
         *  \author David Li
         *  \since 1.12
         */
        public bool UseSSL
        {
            get
            {
                return useSSL;
            }
            set
            {
                useSSL = value;
            }
        }
    }
}
