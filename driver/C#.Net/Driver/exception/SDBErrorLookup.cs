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
using System.IO;

namespace SequoiaDB
{
    class SDBErrorLookup
    {
        private static Dictionary<string, SDBError> mapByType = null;
        private static Dictionary<int, SDBError> mapByCode = null;

        private static void LoadErrorMap()
        {
            mapByType = new Dictionary<string, SDBError>();
            mapByCode = new Dictionary<int, SDBError>();

            foreach (int errCode in Enum.GetValues(typeof(Errors.errors)))
            {
                SDBError error = new SDBError();
                string errType = Enum.GetName(typeof(Errors.errors), errCode);
                error.ErrorCode = errCode;
                error.ErrorType = errType;
                error.ErrorDescription = Errors.descriptions[(-errCode) - 1];

                mapByCode.Add(errCode, error);
                mapByType.Add(errType, error);
            }
        }

        public static string GetErrorDescriptionByType(string errorType)
        {
            if (mapByType == null)
                LoadErrorMap();
            SDBError errObj = mapByType[errorType];
            if (errObj != null)
                return errObj.ErrorDescription;
            return SequoiadbConstants.UNKONWN_DESC;
        }

        public static string GetErrorDescriptionByCode(int errorCode)
        {
            if (mapByType == null)
                LoadErrorMap();
            SDBError errObj = mapByCode[errorCode];
            if (errObj != null)
                return errObj.ErrorDescription;
            return SequoiadbConstants.UNKONWN_DESC;
        }

        public static int GetErrorCodeByType(string errorType)
        {
            if (mapByType == null)
                LoadErrorMap();
            SDBError errObj = mapByType[errorType];
            if (errObj != null)
                return errObj.ErrorCode;
            return SequoiadbConstants.UNKNOWN_CODE;
        }

        public static string GetErrorTypeByCode(int errorCode)
        {
            if (mapByType == null)
                LoadErrorMap();
            SDBError errObj = mapByCode[errorCode];
            if (errObj != null)
                return errObj.ErrorType;
            return SequoiadbConstants.UNKNOWN_TYPE;
        }
    }
}
