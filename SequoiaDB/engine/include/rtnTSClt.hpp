#ifndef RTN_TS_CLT_HPP__
#define RTN_TS_CLT_HPP__

#include "rtnQueryOptions.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   INT32 rtnTSGetCount( const rtnQueryOptions &options,
                        pmdEDUCB *cb, INT64 &count ) ;
}

#endif
