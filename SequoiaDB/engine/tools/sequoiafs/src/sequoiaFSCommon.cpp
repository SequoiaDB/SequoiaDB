#include "sequoiaFSCommon.hpp"
#include "pmdDef.hpp"

using namespace engine;

namespace sequoiafs
{
   INT64 min64(INT64 a, INT64 b)
   {
      if(a > b)
      {
         return b;
      }
      else
      {
         return a;
      }
   }

   INT64 max64(INT64 a, INT64 b)
   {
      if( a < b)
      {
         return b;
      }
      else
      {
         return a;
      }
   }

   INT32 buildDialogPath(CHAR *diaglogPath, CHAR *diaglogPathFromCmd,
                      UINT32 bufSize)
   {
      INT32 rc = SDB_OK;
      CHAR currentPath[OSS_MAX_PATHSIZE + 1] = {0};

      if(bufSize < OSS_MAX_PATHSIZE + 1)
      {
         rc = SDB_INVALIDARG;
         ossPrintf("Path buffer size is too small: %u" OSS_NEWLINE, bufSize);
         goto error;
      }

      rc = ossGetEWD(currentPath, OSS_MAX_PATHSIZE);
      if(rc)
      {
         ossPrintf("Get working directory failed: %d" OSS_NEWLINE, rc);
         goto error;
      }

      rc = engine::utilBuildFullPath(diaglogPathFromCmd, PMD_OPTION_DIAG_PATH,
                                     OSS_MAX_PATHSIZE, diaglogPath);
      if(rc)
      {
         ossPrintf("Build log path failed: %d" OSS_NEWLINE, rc);
         goto error;
      }

      rc = ossMkdir(diaglogPath);
      if(rc)
      {
         if(SDB_FE != rc)
         {
             ossPrintf("Make diralog path [%s] faild: %d" OSS_NEWLINE,
                       diaglogPath, rc);
             goto error;
         }
         else
         {
             rc = SDB_OK;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
}
