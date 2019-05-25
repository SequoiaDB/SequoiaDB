/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = aggrDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2013  JHL  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef AGGRDEF_HPP__
#define AGGRDEF_HPP__

#define AGGR_KEYWORD_PREFIX               '$'
#define AGGR_CL_DEFAULT_ALIAS             "SYS_AGGR_ALIAS"

#define AGGR_KEYWORD_PREFIX_STR           "$"
#define AGGR_GROUP_PARSER_NAME            AGGR_KEYWORD_PREFIX_STR"group"
#define AGGR_MATCH_PARSER_NAME            AGGR_KEYWORD_PREFIX_STR"match"
#define AGGR_SKIP_PARSER_NAME             AGGR_KEYWORD_PREFIX_STR"skip"
#define AGGR_LIMIT_PARSER_NAME            AGGR_KEYWORD_PREFIX_STR"limit"
#define AGGR_SORT_PARSER_NAME             AGGR_KEYWORD_PREFIX_STR"sort"
#define AGGR_PROJECT_PARSER_NAME          AGGR_KEYWORD_PREFIX_STR"project"

#endif