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

   Source File Name = coordSnapshotDef.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_SNAPSHOT_DEF_HPP__
#define COORD_SNAPSHOT_DEF_HPP__

#define COORD_SNAPSHOTDB_INPUT         "{$group:{\
                                                TotalNumConnects:{$sum:\"$TotalNumConnects\"},\
                                                TotalDataRead:{$sum:\"$TotalDataRead\"},\
                                                TotalIndexRead:{$sum:\"$TotalIndexRead\"},\
                                                TotalDataWrite:{$sum:\"$TotalDataWrite\"},\
                                                TotalIndexWrite:{$sum:\"$TotalIndexWrite\"},\
                                                TotalUpdate:{$sum:\"$TotalUpdate\"},\
                                                TotalDelete:{$sum:\"$TotalDelete\"},\
                                                TotalInsert:{$sum:\"$TotalInsert\"},\
                                                ReplUpdate:{$sum:\"$ReplUpdate\"},\
                                                ReplDelete:{$sum:\"$ReplDelete\"},\
                                                ReplInsert:{$sum:\"$ReplInsert\"},\
                                                TotalSelect:{$sum:\"$TotalSelect\"},\
                                                TotalRead:{$sum:\"$TotalRead\"},\
                                                TotalReadTime:{$sum:\"$TotalReadTime\"},\
                                                TotalWriteTime:{$sum:\"$TotalWriteTime\"},\
                                                freeLogSpace:{$sum:\"$freeLogSpace\"},\
                                                vsize:{$sum:\"$vsize\"},\
                                                rss:{$sum:\"$rss\"},\
                                                fault:{$sum:\"$fault\"},\
                                                TotalMapped:{$sum:\"$TotalMapped\"},\
                                                svcNetIn:{$sum:\"$svcNetIn\"},\
                                                svcNetOut:{$sum:\"$svcNetOut\"},\
                                                shardNetIn:{$sum:\"$shardNetIn\"},\
                                                shardNetOut:{$sum:\"$shardNetOut\"},\
                                                replNetIn:{$sum:\"$replNetIn\"},\
                                                replNetOut:{$sum:\"$replNetOut\"},\
                                                ErrNodes:{$mergearrayset:\"$ErrNodes\"}\
                                                }\
                                       }"

#define COORD_SNAPSHOTSYS_INPUT       "{$group:{\
                                                _id:{HostName:\"$HostName\",\
                                                     \"Disk.Name\":\"$Disk.Name\"},\
                                                HostName:\"$HostName\",\
                                                User:\"$CPU.User\",\
                                                Sys:\"$CPU.Sys\",\
                                                Idle:\"$CPU.Idle\",\
                                                Other:\"$CPU.Other\",\
                                                TotalRAM:\"$Memory.TotalRAM\",\
                                                FreeRAM:\"$Memory.FreeRAM\",\
                                                TotalSwap:\"$Memory.TotalSwap\",\
                                                FreeSwap:\"$Memory.FreeSwap\",\
                                                TotalVirtual:\"$Memory.TotalVirtual\",\
                                                FreeVirtual:\"$Memory.FreeVirtual\",\
                                                Name:\"$Disk.Name\",\
                                                TotalSpace:\"$Disk.TotalSpace\",\
                                                FreeSpace:\"$Disk.FreeSpace\",\
                                                ErrNodes:{$mergearrayset:\"$ErrNodes\"}\
                                               }\
                                       }\n\
                                       {$group:{\
                                                _id:\"$HostName\",\
                                                User:\"$User\",\
                                                Sys:\"$Sys\",\
                                                Idle:\"$Idle\",\
                                                Other:\"$Other\",\
                                                TotalRAM:\"$TotalRAM\",\
                                                FreeRAM:\"$FreeRAM\",\
                                                TotalSwap:\"$TotalSwap\",\
                                                FreeSwap:\"$FreeSwap\",\
                                                TotalVirtual:\"$TotalVirtual\",\
                                                FreeVirtual:\"$FreeVirtual\",\
                                                TotalSpace:{$sum:\"$TotalSpace\"},\
                                                FreeSpace:{$sum:\"$FreeSpace\"},\
                                                ErrNodes:{$mergearrayset:\"$ErrNodes\"}\
                                               }\
                                       }\n\
                                       {$group:{\
                                                User:{$sum:\"$User\"},\
                                                Sys:{$sum:\"$Sys\"},\
                                                Idle:{$sum:\"$Idle\"},\
                                                Other:{$sum:\"$Other\"},\
                                                TotalRAM:{$sum:\"$TotalRAM\"},\
                                                FreeRAM:{$sum:\"$FreeRAM\"},\
                                                TotalSwap:{$sum:\"$TotalSwap\"},\
                                                FreeSwap:{$sum:\"$FreeSwap\"},\
                                                TotalVirtual:{$sum:\"$TotalVirtual\"},\
                                                FreeVirtual:{$sum:\"$FreeVirtual\"},\
                                                TotalSpace:{$sum:\"$TotalSpace\"},\
                                                FreeSpace:{$sum:\"$FreeSpace\"},\
                                                ErrNodes:{$mergearrayset:\"$ErrNodes\"}\
                                               }\
                                       }\n\
                                       {$project:{\
                                                CPU:{User:1,Sys:1,Idle:1,Other:1},\
                                                Memory:{TotalRAM:1,FreeRAM:1,TotalSwap:1,\
                                                        FreeSwap:1,TotalVirtual:1,FreeVirtual:1},\
                                                Disk:{TotalSpace:1,FreeSpace:1},\
                                                ErrNodes:1\
                                                 }\
                                       }"

#define COORD_SNAPSHOTCL_INPUT         "{$project:{\
                                                Name:1,\
                                                GroupName:\"$Details.$[0].GroupName\",\
                                                ID:\"$Details.$[0].ID\",\
                                                LogicalID:\"$Details.$[0].LogicalID\",\
                                                Sequence:\"$Details.$[0].Sequence\",\
                                                Indexes:\"$Details.$[0].Indexes\",\
                                                Status:\"$Details.$[0].Status\",\
                                                TotalRecords:\"$Details.$[0].TotalRecords\",\
                                                TotalDataPages:\"$Details.$[0].TotalDataPages\",\
                                                TotalIndexPages:\"$Details.$[0].TotalIndexPages\",\
                                                TotalLobPages:\"$Details.$[0].TotalLobPages\",\
                                                TotalDataFreeSpace:\"$Details.$[0].TotalDataFreeSpace\",\
                                                TotalIndexFreeSpace:\"$Details.$[0].TotalIndexFreeSpace\",\
                                                NodeName:\"$Details.$[0].NodeName\"\
                                                }\
                                       }\n\
                                       {$project:{\
                                                Name:1,\
                                                GroupName:1,\
                                                Details:{ID:1,LogicalID:1,Sequence:1,\
                                                         Indexes:1,Status:1,TotalRecords:1,TotalDataPages:1,\
                                                         TotalIndexPages:1,TotalLobPages:1,TotalDataFreeSpace:1,\
                                                         TotalIndexFreeSpace:1,NodeName:1}\
                                                }\
                                       }\n\
                                       {$group:{\
                                                _id:{Name:\"$Name\",\
                                                     GroupName:\"$GroupName\"},\
                                                Name:{$first:\"$Name\"},\
                                                Group:{$push:\"$Details\"},\
                                                GroupName:{$first:\"$GroupName\"}\
                                                }\
                                       }\n\
                                       {$project:{\
                                                Name:1,\
                                                Details:{GroupName:1,Group:1}\
                                                }\
                                       }\n\
                                       {$group:{\
                                                _id:\"$Name\",\
                                                Name:{$first:\"$Name\"},\
                                                Details:{$push:\"$Details\"}\
                                                }\
                                       }\n\
                                       {$match:{$and:[{Name:{$exists:1}},\
                                                   {Name:{$ne:null}}]}}"

#define COORD_SNAPSHOTCS_INPUT         "{$group:{\
                                                _id:\"$Name\",\
                                                Name:{$first:\"$Name\"},\
                                                PageSize:{$first:\"$PageSize\"},\
                                                LobPageSize:{$first:\"$LobPageSize\"},\
                                                TotalSize:{$sum:\"$TotalSize\"},\
                                                FreeSize:{$sum:\"$FreeSize\"},\
                                                TotalDataSize:{$sum:\"$TotalDataSize\"},\
                                                FreeDataSize:{$sum:\"$FreeDataSize\"},\
                                                TotalIndexSize:{$sum:\"$TotalIndexSize\"},\
                                                FreeIndexSize:{$sum:\"$FreeIndexSize\"},\
                                                TotalLobSize:{$sum:\"$TotalLobSize\"},\
                                                FreeLobSize:{$sum:\"$FreeLobSize\"},\
                                                Collection:{$mergearrayset:\"$Collection\"},\
                                                Group:{$addtoset:\"$GroupName\"}\
                                                }\
                                       }\n\
                                       {$match:{$and:[{Name:{$exists:1}},\
                                                   {Name:{$ne:null}}]}}"

#define COORD_SNAPSHOTSESS_INPUT       "{$sort:{SessionID:1}}\n\
                                       {$match:{$and:[{SessionID:{$exists:1}},\
                                                   {SessionID:{$ne:null}}]}}"

#define COORD_SNAPSHOTSESSCUR_INPUT    COORD_SNAPSHOTSESS_INPUT

#define COORD_SNAPSHOTCONTEXTS_INPUT   "{$sort:{SessionID:1}}\n\
                                          {$match:{$and:[{Contexts:{$exists:1}},\
                                                   {Contexts:{$ne:null}}]}}"

#define COORD_SNAPSHOTCONTEXTSCUR_INPUT   COORD_SNAPSHOTCONTEXTS_INPUT


#endif // COORD_SNAPSHOT_DEF_HPP__
