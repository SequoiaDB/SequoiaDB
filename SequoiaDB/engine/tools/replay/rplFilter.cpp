/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = rplFilter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplFilter.hpp"
#include "rplUtil.hpp"
#include "dpsLogRecordDef.hpp"
#include "pd.hpp"
#include "ossFile.hpp"
#include "utilStr.hpp"
#include "../bson/bsonobjiterator.h"
#include <iostream>

using namespace engine;

namespace replay
{

   #define RPL_FILTER_FILE             "File"
   #define RPL_FILTER_EXCL_FILE        "ExclFile"
   #define RPL_FILTER_CS               "CS"
   #define RPL_FILTER_EXCL_CS          "ExclCS"
   #define RPL_FILTER_CL               "CL"
   #define RPL_FILTER_EXCL_CL          "ExclCL"
   #define RPL_FILTER_OP               "OP"
   #define RPL_FILTER_EXCL_OP          "ExclOP"
   #define RPL_FILTER_MIN_LSN          "MinLSN"
   #define RPL_FILTER_MAX_LSN          "MaxLSN"

   struct FilterField
   {
      const char* field;
      set<string>& fieldSet;
   };

   static BOOLEAN isValidFilterField(const string& field)
   {
      static const CHAR* fields[] =
      {
         RPL_FILTER_FILE,
         RPL_FILTER_EXCL_FILE,
         RPL_FILTER_CS,
         RPL_FILTER_EXCL_CS,
         RPL_FILTER_CL,
         RPL_FILTER_EXCL_CL,
         RPL_FILTER_OP,
         RPL_FILTER_EXCL_OP,
         RPL_FILTER_MIN_LSN,
         RPL_FILTER_MAX_LSN
      };

      const INT32 fieldNum = sizeof(fields) / sizeof(fields[0]);

      BOOLEAN valid = FALSE;

      for (INT32 i = 0; i < fieldNum; i++)
      {
         if (field == fields[i])
         {
            valid = TRUE;
            break;
         }
      }

      return valid;
   }

   static BOOLEAN isValidOPName(const string& op)
   {
      static const CHAR* ops[] =
      {
         RPL_LOG_OP_INSERT,
         RPL_LOG_OP_UPDATE,
         RPL_LOG_OP_DELETE,
         RPL_LOG_OP_TRUNCATE_CL,
         RPL_LOG_OP_CREATE_CS,
         RPL_LOG_OP_DELETE_CS,
         RPL_LOG_OP_CREATE_CL,
         RPL_LOG_OP_DELETE_CL,
         RPL_LOG_OP_CREATE_IX,
         RPL_LOG_OP_DELETE_IX,
         RPL_LOG_OP_LOB_WRITE,
         RPL_LOG_OP_LOB_REMOVE,
         RPL_LOG_OP_LOB_UPDATE,
         RPL_LOG_OP_LOB_TRUNCATE,
         RPL_LOG_OP_DUMMY,
         RPL_LOG_OP_CL_RENAME,
         RPL_LOG_OP_TS_COMMIT,
         RPL_LOG_OP_TS_ROLLBACK,
         RPL_LOG_OP_INVALIDATE_CATA,
         RPL_LOG_OP_CS_RENAME,
         RPL_LOG_OP_POP
      };

      const INT32 opNum = sizeof(ops) / sizeof(ops[0]);

      BOOLEAN valid = FALSE;

      for (INT32 i = 0; i < opNum; i++)
      {
         if (op == ops[i])
         {
            valid = TRUE;
            break;
         }
      }

      return valid;
   }

   static BOOLEAN isValidFilterOP(UINT16 type)
   {
      switch(type)
      {
      case LOG_TYPE_DATA_INSERT:
      case LOG_TYPE_DATA_UPDATE:
      case LOG_TYPE_DATA_DELETE:
      case LOG_TYPE_CL_TRUNC:
      case LOG_TYPE_DATA_POP:
         return TRUE;
      default:
         return FALSE;
      }
   }

   Filter::Filter()
   {
      _minLSN = DPS_INVALID_LSN_OFFSET;
      _maxLSN = DPS_INVALID_LSN_OFFSET;
   }

   Filter::~Filter()
   {
   }

   INT32 Filter::init(const BSONObj& filterObj)
   {
      INT32 rc = SDB_OK;
      INT64 value;

      FilterField fields[] =
      {
         {RPL_FILTER_FILE,       _files},
         {RPL_FILTER_EXCL_FILE,  _exclFiles},
         {RPL_FILTER_CS,         _cs},
         {RPL_FILTER_EXCL_CS,    _exclCS},
         {RPL_FILTER_CL,         _cl},
         {RPL_FILTER_EXCL_CL,    _exclCL},
         {RPL_FILTER_OP,         _op},
         {RPL_FILTER_EXCL_OP,    _exclOP},
      };

      const INT32 fieldNum = sizeof(fields) / sizeof(fields[0]);

      rc = _checkFilterField(filterObj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (INT32 i = 0; i < fieldNum; i++)
      {
         rc = _readStringArray(filterObj, fields[i].field, fields[i].fieldSet);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = _checkFilterOP(_op);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _checkFilterOP(_exclOP);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (_op.empty())
      {
         _op.insert(RPL_LOG_OP_INSERT);
         _op.insert(RPL_LOG_OP_UPDATE);
         _op.insert(RPL_LOG_OP_DELETE);
         _op.insert(RPL_LOG_OP_TRUNCATE_CL);
         _op.insert(RPL_LOG_OP_POP);
      }

      value = DPS_INVALID_LSN_OFFSET;
      rc = _readInt64(filterObj, RPL_FILTER_MIN_LSN, value);
      if (SDB_OK != rc)
      {
         goto error;
      }
      _minLSN = (DPS_LSN_OFFSET)value;

      value = DPS_INVALID_LSN_OFFSET;
      rc = _readInt64(filterObj, RPL_FILTER_MAX_LSN, value);
      if (SDB_OK != rc)
      {
         goto error;
      }
      _maxLSN = (DPS_LSN_OFFSET)value;

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN Filter::isFiltered(engine::dpsArchiveFile& file)
   {
      engine::dpsArchiveHeader* archiveHeader = file.getArchiveHeader();
      string fileName = engine::ossFile::getFileName(file.path());

      if (_isFileFiltered(fileName))
      {
         return TRUE;
      }

      if (DPS_INVALID_LSN_OFFSET != _minLSN)
      {
         if (archiveHeader->endLSN.compareOffset(_minLSN) <= 0)
         {
            return TRUE;
         }
      }

      if (DPS_INVALID_LSN_OFFSET != _maxLSN)
      {
         if (archiveHeader->startLSN.compareOffset(_maxLSN) > 0)
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::isFiltered(engine::dpsLogFile& file)
   {
      string fileName = engine::ossFile::getFileName(file.path());

      if (_isFileFiltered(fileName))
      {
         return TRUE;
      }

      if (DPS_INVALID_LSN_OFFSET != _minLSN)
      {
         dpsLogHeader& header = file.header();
         if (DPS_INVALID_LOG_FILE_ID != header._logID)
         {
            DPS_LSN_OFFSET endLSN = header._fileSize * (header._logID + 1);
            if (endLSN <= _minLSN)
            {
               return TRUE;
            }
         }
      }

      if (DPS_INVALID_LSN_OFFSET != _maxLSN)
      {
         if (file.getFirstLSN().compareOffset(_maxLSN) > 0)
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::isFiltered(const dpsLogRecord& log, BOOLEAN dump)
   {
      const dpsLogRecordHeader& head = log.head();
      DPS_LSN_OFFSET lsn = head._lsn;
      string op;

      // when dump, don't invalidate op
      if (!dump && !isValidFilterOP(head._type))
      {
         return TRUE;
      }

      op = getOPName(head._type);
      if (_isOPFiltered(op))
      {
         return TRUE;
      }

      if (_isLSNFiltered(lsn))
      {
         return TRUE;
      }

      if (LOG_TYPE_CS_CRT == head._type ||
          LOG_TYPE_CS_DELETE == head._type)
      {
         DPS_TAG tag = (LOG_TYPE_CS_CRT == head._type) ?
                       (DPS_TAG)DPS_LOG_CSCRT_CSNAME :
                       (DPS_TAG)DPS_LOG_CSDEL_CSNAME;
         dpsLogRecord::iterator it = log.find(tag);
         if (it.valid())
         {
            string csName = it.value();
            if (_isCSFiltered(csName))
            {
               return TRUE;
            }
         }
      }

      if (!_exclCL.empty() ||
          !_cl.empty() ||
          !_exclCS.empty() ||
          !_cs.empty())
      {
         dpsLogRecord::iterator it = log.find(DPS_LOG_PUBLIC_FULLNAME);
         if (it.valid())
         {
            string fullName = it.value();
            if (_isCLFiltered(fullName))
            {
               return TRUE;
            }

            vector<string> cscl = utilStrSplit(fullName, ".");
            if (cscl.size() == 2)
            {
               string csName = cscl.at(0);
               if (_isCSFiltered(csName))
               {
                  return TRUE;
               }
            }
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::lessThanMinLSN(DPS_LSN_OFFSET lsn)
   {
      if (DPS_INVALID_LSN_OFFSET != _minLSN)
      {
         if (lsn < _minLSN)
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::largerThanMaxLSN(DPS_LSN_OFFSET lsn)
   {
      if (DPS_INVALID_LSN_OFFSET != _maxLSN)
      {
         if (lsn >= _maxLSN)
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::_isFileFiltered(const string& fileName)
   {
      // this file is excluded
      if (!_exclFiles.empty())
      {
         if (_exclFiles.end() != _exclFiles.find( fileName ))
         {
            return TRUE;
         }
      }

      // this file is not included
      if (!_files.empty())
      {
         if (_files.end() == _files.find( fileName ))
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::_isCSFiltered(const string& csName)
   {
      if (!_exclCS.empty())
      {
         if (_exclCS.end() != _exclCS.find(csName))
         {
            return TRUE;
         }
      }

      if (!_cs.empty())
      {
         if (_cs.end() == _cs.find(csName))
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::_isCLFiltered(const string& fullName)
   {
      if (!_exclCL.empty())
      {
         if (_exclCL.end() != _exclCL.find(fullName))
         {
            return TRUE;
         }
      }

      if (!_cl.empty())
      {
         if (_cl.end() == _cl.find(fullName))
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::_isOPFiltered(const string& op)
   {
      if (!_exclOP.empty())
      {
         if (_exclOP.end() != _exclOP.find(op))
         {
            return TRUE;
         }
      }

      if (!_op.empty())
      {
         if (_op.end() == _op.find(op))
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   BOOLEAN Filter::_isLSNFiltered(DPS_LSN_OFFSET lsn)
   {
      if (lessThanMinLSN(lsn))
      {
         return TRUE;
      }

      if (largerThanMaxLSN(lsn))
      {
         return TRUE;
      }

      return FALSE;
   }

   INT32 Filter::_checkFilterField(const BSONObj& filterObj)
   {
      INT32 rc = SDB_OK;

      BSONObjIterator it(filterObj);
      while (it.more())
      {
         BSONElement ele = it.next();
         string field = string(ele.fieldName());
         if (!isValidFilterField(field))
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid filter field: %s", field.c_str());
            std::cerr << "Invalid filter field: "
                      << field
                      << std::endl;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Filter::_checkFilterOP(const set<string>& ops)
   {
      INT32 rc = SDB_OK;

      set<string>::const_iterator it = ops.begin();
      for (; it != ops.end(); it++)
      {
         string op = *it;
         if (!isValidOPName(op))
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid filter op: %s", op.c_str());
            std::cerr << "Invalid filter op: "
                      << op
                      << std::endl;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Filter::_readStringArray(const BSONObj& filterObj,
                             const string& fieldName,
                             set<string>& out)
   {
      INT32 rc = SDB_OK;
      BSONElement ele;

      ele = filterObj.getField(fieldName);
      if (ele.eoo())
      {
         goto done;
      }
      else if (ele.type() != Array)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Filter[%s] should be Array", fieldName.c_str());
         std::cerr << "Filter["
                   << fieldName
                   << "] should be Array"
                   << std::endl;
         goto error;
      }

      {
         BSONObjIterator it(ele.embeddedObject());
         while (it.more())
         {
            BSONElement e = it.next();
            if (e.type() != String)
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "Element should be String in %s",
                      fieldName.c_str());
               std::cerr << "Element should be String in "
                         << fieldName
                         << std::endl;
               goto error;
            }

            out.insert(e.String());
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Filter::_readInt64(const BSONObj& filterObj,
                         const string& fieldName,
                         INT64& out)
   {
      INT32 rc = SDB_OK;
      BSONElement ele;

      ele = filterObj.getField(fieldName);
      if (ele.eoo())
      {
         goto done;
      }
      else if (ele.type() != NumberInt && ele.type() != NumberLong)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Filter[%s] should be integer", fieldName.c_str());
         std::cerr << "Filter["
                   << fieldName
                   << "] should be integer"
                   << std::endl;
         goto error;
      }

      out = ele.numberLong();

   done:
      return rc;
   error:
      goto done;
   }
}

