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

   Source File Name = catGTSDef.hpp

   Descriptive Name = GTS definition

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/13/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_GTS_DEF_HPP_
#define CAT_GTS_DEF_HPP_

#include "msgDef.h"

#define GTS_SYS_COLLECTION_SPACE_NAME        "SYSGTS"

// SEQUENCE

#define CAT_SEQUENCE_NAME              FIELD_NAME_SEQUENCE_NAME
#define CAT_SEQUENCE_OID               FIELD_NAME_SEQUENCE_OID
#define CAT_SEQUENCE_ID                FIELD_NAME_SEQUENCE_ID
#define CAT_SEQUENCE_VERSION           FIELD_NAME_VERSION
#define CAT_SEQUENCE_CURRENT_VALUE     FIELD_NAME_CURRENT_VALUE
#define CAT_SEQUENCE_INCREMENT         FIELD_NAME_INCREMENT
#define CAT_SEQUENCE_START_VALUE       FIELD_NAME_START_VALUE
#define CAT_SEQUENCE_MIN_VALUE         FIELD_NAME_MIN_VALUE
#define CAT_SEQUENCE_MAX_VALUE         FIELD_NAME_MAX_VALUE
#define CAT_SEQUENCE_CACHE_SIZE        FIELD_NAME_CACHE_SIZE
#define CAT_SEQUENCE_ACQUIRE_SIZE      FIELD_NAME_ACQUIRE_SIZE
#define CAT_SEQUENCE_CYCLED            FIELD_NAME_CYCLED
#define CAT_SEQUENCE_INTERNAL          FIELD_NAME_INTERNAL
#define CAT_SEQUENCE_INITIAL           FIELD_NAME_INITIAL
#define CAT_SEQUENCE_NEXT_VALUE        FIELD_NAME_NEXT_VALUE
#define CAT_SEQUENCE_EXPECT_VALUE      FIELD_NAME_EXPECT_VALUE

#define GTS_SEQUENCE_COLLECTION_NAME         GTS_SYS_COLLECTION_SPACE_NAME".SEQUENCES"
#define GTS_SEQUENCE_NAME_INDEX              "{name:\"name_index\",key: {\""CAT_SEQUENCE_NAME"\": 1}, unique: true, enforced: true}"

#endif /* CAT_GTS_DEF_HPP_ */

