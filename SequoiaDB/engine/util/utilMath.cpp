
#include "utilMath.hpp"

namespace engine
{

   BOOLEAN utilAddIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if( l > 0 && r > 0 && result < 0 )
      {
         ret = TRUE ;
      }
      else if( l < 0 && r < 0 && result >= 0 )
      {
         ret = TRUE ;
      }

      return ret ;
   }

   BOOLEAN utilSubIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if( l >= 0 && r < 0 && result < 0 )
      {
         ret = TRUE ;
      }
      else if( l < 0 && r > 0 && result > 0 )
      {
         ret = TRUE ;
      }

      return ret ;
   }

   BOOLEAN utilMulIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if ( l != (INT64) ((INT32) l) || r != (INT64) ((INT32) r) )
      {
         if ( r != 0 &&
              ( ( r == -1 && l < 0 && result < 0 ) ||
                result / r != l ) )
         {
            ret = TRUE ;
         }
      }
      return ret ;
   }

   BOOLEAN utilDivIsOverflow( INT64 l, INT64 r )
   {
      BOOLEAN ret = FALSE ;
      if ( OSS_SINT64_MIN == l && -1 == r )
      {
         ret = TRUE ;
      }
      return ret ;
   }

}
