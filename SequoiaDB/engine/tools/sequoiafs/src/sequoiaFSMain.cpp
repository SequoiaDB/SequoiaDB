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

   Source File Name = sequoiaFSMain.hpp

   Descriptive Name = sequoiafs main entry.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who Description
   ====== =========== === ==============================================
        03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/

#include"sequoiaFSMain.hpp"

using namespace engine;
using namespace std;
using namespace sequoiafs;

sequoiaFS *sfs = new sequoiaFS();

#define SDB_LOB_VALID_CHECK() \
{ \
   if(!sfs)\
      return -1;\
}

INT32 sfsGetattr(const CHAR *path, struct stat *stat)
{
   SDB_LOB_VALID_CHECK();
   return sfs->getattr(path, stat);
}
INT32 sfsReadlink(const CHAR *path, CHAR * link, size_t size)
{
   SDB_LOB_VALID_CHECK();
   return sfs->readlink(path, link, size);
}
INT32 sfsGetdir(const CHAR *path, fuse_dirh_t dirh, fuse_dirfil_t dirfill)
{
   SDB_LOB_VALID_CHECK();
   return sfs->getdir(path, dirh, dirfill);
}
INT32 sfsMknod(const CHAR *path, mode_t mode, dev_t dev)
{
   SDB_LOB_VALID_CHECK();
   return sfs->mknod(path, mode, dev);
}
INT32 sfsMkdir(const CHAR *path, mode_t mode)
{
   SDB_LOB_VALID_CHECK();
   return sfs->mkdir(path, mode);
}
INT32 sfsUnlink(const CHAR *path)
{
   SDB_LOB_VALID_CHECK();
   return sfs->unlink(path);
}
INT32 sfsRmdir(const CHAR *path)
{
   SDB_LOB_VALID_CHECK();
   return sfs->rmdir(path);
}
INT32 sfsSymlink(const CHAR *path, const CHAR *link)
{
   SDB_LOB_VALID_CHECK();
   return sfs->symlink(path, link);
}
INT32 sfsRename(const CHAR *path, const CHAR *newpath)
{
   SDB_LOB_VALID_CHECK();
   return sfs->rename(path, newpath);
}
INT32 sfsLink(const CHAR *path, const CHAR *newpath)
{
   SDB_LOB_VALID_CHECK();
   return sfs->link(path, newpath);
}
INT32 sfsChmod(const CHAR *path, mode_t mode)
{
   SDB_LOB_VALID_CHECK();
   return sfs->chmod(path, mode);
}
INT32 sfsChown(const CHAR *path, uid_t uid, gid_t gid)
{
   SDB_LOB_VALID_CHECK();
   return sfs->chown(path, uid, gid);
}
INT32 sfsTruncate(const CHAR *path, off_t newsize)
{
   SDB_LOB_VALID_CHECK();
   return sfs->truncate(path, newsize);
}
INT32 sfsUtime(const CHAR *path, struct utimbuf *ubuf)
{
   SDB_LOB_VALID_CHECK();
   return sfs->utime(path, ubuf);
}
INT32 sfsOpen(const CHAR *path, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->open(path, fi);
}
INT32 sfsRead(const CHAR *path, CHAR *buf, size_t size, off_t offset ,
              struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->read(path, buf, size, offset, fi);
}
INT32 sfsWrite(const CHAR *path, const CHAR *buf, size_t size, off_t offset,
               struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->write(path, buf, size, offset, fi);
}
INT32 sfsStatfs(const CHAR *path, struct statvfs *statfs)
{
   SDB_LOB_VALID_CHECK();
   return sfs->statfs(path, statfs);
}
INT32 sfsFlush(const CHAR *path, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->flush(path, fi);
}
INT32 sfsRelease(const CHAR *path, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->release(path, fi);
}
INT32 sfsFsync(const CHAR *path, INT32 datasync, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->fsync(path, datasync, fi);
}
INT32 sfsSetxattr(const CHAR *path, const CHAR *name, const CHAR *value,
                  size_t size, INT32 flags){return 0;}
INT32 sfsGetxattr(const CHAR *path, const CHAR *name, CHAR *value,
                  size_t size){return 0;}
INT32 sfsListxattr(const CHAR *path, CHAR *list, size_t size){return 0;}
INT32 sfsRemovexattr(const CHAR *path, const CHAR *name){return 0;}
INT32 sfsOpendir(const CHAR *path, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->opendir(path, fi);
}
INT32 sfsReaddir(const CHAR *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->readdir(path,buf,filler,offset,fi);

}
INT32 sfsReleasedir(const CHAR *path, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->releasedir(path, fi);
}
void *sfsInit(struct fuse_conn_info *conn){return (void *)0;};
void sfsDestroy(void *){};
INT32 sfsFsyncdir(const CHAR *path, INT32 datasync, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->fsyncdir(path, datasync, fi);
}
INT32 sfsAccess(const CHAR *path, INT32 mask)
{
   SDB_LOB_VALID_CHECK();
   return sfs->access(path, mask);

}
INT32 sfsCreate(const CHAR *path, mode_t mode, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->create(path, mode, fi);
}
INT32 sfsFtruncate(const CHAR *path, off_t offset, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->ftruncate(path, offset, fi);
}
INT32 sfsFgetattr(const CHAR *path, struct stat *buf, struct fuse_file_info *fi)
{
   SDB_LOB_VALID_CHECK();
   return sfs->fgetattr(path, buf, fi);
}
INT32 sfsLock(const CHAR *path, struct fuse_file_info *fi, INT32 cmd,
          struct flock *lock)
{
   SDB_LOB_VALID_CHECK();
   return sfs->lock(path, fi, cmd, lock);
}
INT32 sfsUtimens(const CHAR *path, const struct timespec tv[2])
{
   SDB_LOB_VALID_CHECK();
   return sfs->utimens(path, tv);
}
INT32 sfsBmap(const CHAR *path, size_t blocksize, uint64_t *idx)
{
   SDB_LOB_VALID_CHECK();
   return sfs->bmap(path, blocksize, idx);
}
INT32 sfsIoctl(const CHAR *path, INT32 cmd, void *arg,
               struct fuse_file_info *fi, UINT32 flags, void *data)
{
   SDB_LOB_VALID_CHECK();
   return sfs->ioctl(path, cmd, arg, fi, flags, data);
}
INT32 sfsPoll(const CHAR *path, struct fuse_file_info *fi,
              struct fuse_pollhandle *ph, unsigned *reventsp){return 0;}
INT32 sfsWrite_buf(const CHAR *path, struct fuse_bufvec *buf, off_t off,
                   struct fuse_file_info *fi){return 0;}
INT32 sfsRead_buf(const CHAR *path, struct fuse_bufvec **bufp,
                  size_t size, off_t off, struct fuse_file_info *fi){return 0;}
INT32 sfsFlock(const CHAR *path, struct fuse_file_info *fi, INT32 op)
{
   SDB_LOB_VALID_CHECK();
   return sfs->flock(path, fi, op);
}
INT32 sfsFallocate(const CHAR *path, INT32 mode, off_t offset, off_t value,
                   struct fuse_file_info *fi){return 0;}


struct fuse_operations sfsFuseOper=
{
   sfsGetattr,//Getattr
   sfsReadlink,//Readlink  -->not finish
   sfsGetdir,//Getdir  -->not finish
   sfsMknod,//Mknod  -->not finish
   sfsMkdir,//Mkdir  -->not finish
   sfsUnlink,//Unlink
   sfsRmdir,//Rmdir  -->not finish
   sfsSymlink,//Symlink  -->not finish
   sfsRename,//Rename
   sfsLink,//Link
   sfsChmod,//Chmod  -->not finish
   sfsChown,//Chown  -->not finish
   sfsTruncate,//Truncate   -->not finish
   sfsUtime,//Utime
   sfsOpen,//Open
   sfsRead,//Read
   sfsWrite,//Write
   sfsStatfs,//Statfs  -->not finish
   sfsFlush,//Flush
   sfsRelease,//Release
   sfsFsync,//Fsync
   sfsSetxattr,//Setxattr  -->not finish
   sfsGetxattr,//Getxattr  -->not finish
   sfsListxattr,//Listxattr  -->not finish
   sfsRemovexattr,//Removexatt  -->not finish
   sfsOpendir,//Opendir
   sfsReaddir,//Readdir
   sfsReleasedir,//Releasedir
   sfsFsyncdir,//Fsyncdir  -->not finish
   sfsInit,//init  -->not finish
   sfsDestroy,//destroy  -->not finish
   sfsAccess,//Access
   sfsCreate,//Create
   sfsFtruncate,//Ftruncate  -->not finish
   sfsFgetattr,//Fgetattr  --only called after create, in some cases, if not implemented,
               // call getattr after create will not get the lob, so called this instead
   sfsLock,//Lock  -->not finish
   sfsUtimens,//Utimens  -->not finish
   sfsBmap,//Bmap  -->not finish
   0,//flag_nullpath_ok:1
#if SEQUOIAFS_SUPPORT_FUSE_VERSION == 0x0294
   0,//flag_nopath:1
   0,//flag_utime_omit_ok:1
#endif
   0,//flag_reserved:29
   sfsIoctl,//Ioctl  -->not finish
   sfsPoll,//Poll  -->not finish
#if SEQUOIAFS_SUPPORT_FUSE_VERSION == 0x0294
   0,//Write_buf  -->not finish  (if init, then will call write buf, maybe core)
   0,//Read_buf  -->not finish   (if init, then will call read buf, maybe core)
   sfsFlock,//Flock  -->not finish
   sfsFallocate,//Fallocate  -->not finish
#endif
};

 struct sfsOptionInfo
 {
   CHAR *mountpoint;
   INT32 is_help;
   INT32 is_help_sfs;
   INT32 is_version;
};

enum
{
   SDB_KEY_HELP,
   SDB_KEY_HELP_SFS,
   SDB_KEY_AUTOCREATE,
   SDB_KEY_VERSION
};

#define LOB_OPTION(t, p) { t, offsetof(struct sfsOptionInfo, p), 1}

static const struct fuse_opt lobOptions[]=
{
   FUSE_OPT_KEY("-h", SDB_KEY_HELP),
   FUSE_OPT_KEY("--help", SDB_KEY_HELP),
   FUSE_OPT_KEY("--helpsfs", SDB_KEY_HELP_SFS),
   FUSE_OPT_KEY("-v", SDB_KEY_VERSION),
   FUSE_OPT_KEY("--version", SDB_KEY_VERSION),
   FUSE_OPT_END
};

static INT32 sfsProcessArg(void *data, const CHAR *arg, INT32 key,
                           struct fuse_args *outargs)
{
   struct sfsOptionInfo *option = (sfsOptionInfo*) data;
   (void) outargs;
   (void)arg;
   INT32 rc = 0;

   switch (key)
   {
      case SDB_KEY_HELP:
         option->is_help = 1;
         goto done;
      case SDB_KEY_HELP_SFS:
         option->is_help_sfs = 1;
         goto done;
      case SDB_KEY_VERSION:
         option->is_version = 1;
         goto done;
      case FUSE_OPT_KEY_NONOPT:
         if (!option->mountpoint)
         {
            CHAR mountpoint[PATH_MAX];
            if (realpath(arg, mountpoint) == NULL)
            {
               fprintf(stderr,
                  "fuse: bad mount point `%s': %s\n",
                  arg, strerror(errno));
               rc = -1;
               goto error;
            }
            return fuse_opt_add_opt(&option->mountpoint, mountpoint);
         }
         else
         {
            fprintf(stderr, "fuse: invalid argument `%s'\n", arg);
            rc = -1;
            goto error;
         }
      default:
         rc = 1;
   }
done:
   return rc;
error:
   goto done;
}

INT32 sfsOptInitArgs(struct fuse_args *args)
{
   fuse_args *arg;
   INT32 rc = SDB_OK;

   arg = (fuse_args*)SDB_OSS_MALLOC(sizeof(fuse_args));
   arg->argc = 0;
   if(!arg)
   {
      std::cerr<<"Failed to malloc mem for argv"<<endl;
      rc = SDB_OOM;
      goto error;
   }
   arg->allocated = 1;
   args = arg;

done:
   return rc;
error:
   goto done;
}

INT32 main(INT32 argc, CHAR *argv[])
{
   INT32 rc = SDB_OK ;
   INT32 rc2 = SDB_OK ;
   struct fuse_args fuseArgs = {0};
   CHAR optionTemp[OSS_MAX_PATHSIZE] = {0};
   vector<string> options4fuse;
   string option;
   struct sfsOptionInfo lobFuseOption = {0};
   CHAR pidName[OSS_MAX_PATHSIZE + 1];
   OSSFILE pidFile ;
   BOOLEAN isCreatePidFile = FALSE;
   _ossCmdRunner runner ;

   rc = sfsOptInitArgs(&fuseArgs);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to init args(error=%d), exit."OSS_NEWLINE, rc);
      goto error;
   }
   ossMemset(optionTemp, 0, OSS_MAX_PATHSIZE);
   sprintf(optionTemp, "%s", argv[0]);
   fuse_opt_add_arg(&fuseArgs, optionTemp);

   rc = sfs->init(argc, argv, &options4fuse);
   if(SDB_OK != rc)
   {
      if(-ENOENT == rc)
      {
         ossPrintf("The cl:%s does not exist, exit."OSS_NEWLINE,
                   sfs->_collection.c_str());
      }

      rc = sfs->closeDataSource();
      if(SDB_OK != rc)
      {
         ossPrintf("Failed to start close conn pool(error=%d), "
                   "exit."OSS_NEWLINE, rc);
         goto error;
      }
      ossPrintf("Failed to init, exit."OSS_NEWLINE);
      goto error;
   }

   for(UINT32 i=0; i< options4fuse.size(); i++)
   {
      option = options4fuse[i];
      fuse_opt_add_arg(&fuseArgs, option.c_str());
   }

   rc = fuse_opt_parse(&fuseArgs, &lobFuseOption, lobOptions, sfsProcessArg);
   if(-1 == rc)
   {
      ossPrintf("Failed to parse fuse option(error=%d), exit."OSS_NEWLINE, rc);
      goto error;
   }

   if(lobFuseOption.is_help_sfs || lobFuseOption.is_version)
   {
      goto done;
   }

   sfs->_mountpoint = lobFuseOption.mountpoint;
   rc = fuse_opt_add_arg(&fuseArgs, lobFuseOption.mountpoint);
   if(0 != rc)
   {
      PD_LOG( PDERROR, "Failed to add arg:%s, rc:%d", lobFuseOption.mountpoint, rc ) ;
      ossPrintf("Failed to add arg:%s (error=%d), exit."OSS_NEWLINE,
                lobFuseOption.mountpoint, rc);
      goto error;
   }
   //write mount map history
   rc = sfs->writeMapHistory(sfs->getHosts());
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to write map history collection, rc:%d", rc ) ;
      ossPrintf("Failed to write map history collection(error=%d), "
                "exit."OSS_NEWLINE, rc);
      goto error;
   }

   //close datasource to avoid the bgtask threads being closed by fuse_main,
   //start when first called getattr,
   rc = sfs->disableDataSource();
   if(SDB_OK != rc)
   {
      goto error;
   }

   //Alias is unique. Mountpoints can not use the same alias.
   if(ossStrlen((sfs->getOptionMgr())->getAlias()))
   {
      CHAR mount[512] = {0};
      UINT32 exitCode = 0 ;
      ossSnprintf( mount, sizeof( mount ),
          "mount -t fuse.sequoiafs | grep '%s('", (sfs->getOptionMgr())->getAlias() );
      rc = runner.exec( mount, exitCode,
                     FALSE, -1, FALSE, NULL, TRUE ) ;
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "mount query failed. cmd:(%s), rc:%d",  mount, rc ) ;
         ossPrintf("mount query failed. cmd:(%s). errorcode:(%d), exit."OSS_NEWLINE, mount, rc);
         rc = SDB_OPERATION_CONFLICT;
         goto error;
      }
      if ( SDB_OK == exitCode )
      {
         PD_LOG( PDERROR, "The alias(%s) is already in use.",  (sfs->getOptionMgr())->getAlias() ) ;
         ossPrintf("The alias(%s) is already in use, exit."OSS_NEWLINE, (sfs->getOptionMgr())->getAlias() );
         rc = SDB_OPERATION_CONFLICT;
         goto error;
      }
   }

   //Create a pidfile in diagpath and lock it.
   //When the process is exit, the pidfile will be unlocked and deleted.
   if (ossStrlen((sfs->getOptionMgr())->getDiaglogPath()))
   {  
      rc = engine::utilBuildFullPath((sfs->getOptionMgr())->getDiaglogPath(), SDB_SEQUOIAFS_PID_FILE_NAME,
                                  OSS_MAX_PATHSIZE, pidName);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "get pidName failed, rc: %d",  rc ) ;
         ossPrintf("get pidName failed, rc: %d."OSS_NEWLINE, rc);
         goto error;
      }
      else 
      {
         rc = ossAccess( pidName, OSS_MODE_READ );
         if(SDB_OK == rc)
         {
            rc = ossOpen ( pidName, OSS_READONLY, OSS_RU|OSS_RG, pidFile ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Open pidfile(%s) failed, rc: %d", pidName, rc ) ;
               ossPrintf("Open pidfile(%s) failed, exit."OSS_NEWLINE, pidName);
               rc = SDB_OPERATION_CONFLICT;
               goto error;
            }
            rc = ossLockFile ( &pidFile, OSS_LOCK_EX ) ;
            if ( SDB_OK != rc )
            {
               CHAR oldPid[10] = {0};
               INT64 readSize = 0;
               ossRead( &pidFile, oldPid, (INT64)(sizeof(oldPid) - 1), &readSize);
               ossClose( pidFile );
               PD_LOG( PDERROR, "Lock pidfile(%s) failed, maybe the mountpoint(%s)"
                       "has been mounted by other process(%s), rc: %d", 
                       pidName, lobFuseOption.mountpoint, oldPid, rc ) ;
               ossPrintf("Lock pidfile(%s) failed, maybe the mountpoint(%s) has "
                         "been mounted by other process(%s), exit."OSS_NEWLINE, 
                         pidName, lobFuseOption.mountpoint, oldPid);
               rc = SDB_OPERATION_CONFLICT;
               goto error;
            }
            ossLockFile ( &pidFile, OSS_LOCK_UN ) ;
            ossClose( pidFile );
            engine::removePIDFile( pidName ) ;
         }
         
         rc = engine::createPIDFile( pidName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to create pid file(%s), rc: %d", pidName, rc ) ;
            ossPrintf("Failed to create pid file(%s), exit."OSS_NEWLINE, pidName);
            goto error;
         }
         isCreatePidFile = TRUE;
         rc = ossOpen ( pidName, OSS_READONLY, OSS_RU|OSS_RG, pidFile ) ;
         if ( SDB_OK != rc )
         {
            engine::removePIDFile( pidName ) ;
            PD_LOG( PDERROR, "open pidfile(%s) failed, rc:%d.", pidName, rc ) ;
            ossPrintf("open pidfile(%s) failed, exit."OSS_NEWLINE, pidName);
            rc = SDB_OPERATION_CONFLICT;
            goto error;
         }
         rc = ossLockFile ( &pidFile, OSS_LOCK_EX ) ;
         if ( SDB_OK != rc )
         {
            ossClose( pidFile );
            engine::removePIDFile( pidName ) ;
            PD_LOG( PDERROR, "Lock pidfile(%s) failed, rc:%d.", pidName, rc ) ;
            ossPrintf("Lock pidfile(%s) failed, exit."OSS_NEWLINE, pidName);
            rc = SDB_OPERATION_CONFLICT;
            goto error;
         }
      }
   }

   rc = fuse_main(fuseArgs.argc, fuseArgs.argv, &sfsFuseOper, NULL);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to start fuse main, rc:%d.", rc ) ;
      ossPrintf("Failed to start fuse main(error=%d), exit."OSS_NEWLINE, rc);
   }
   else
   {
      rc2 = (sfs->getOptionMgr())->save(); 
      if(SDB_OK != rc2)
      {
         PD_LOG( PDWARNING, "Failed to start close conn pool, rc:%d.", rc2 ) ;
         ossPrintf("Failed to save config (error=%d), exit."OSS_NEWLINE, rc2);
      }
   }
   
   rc2 = sfs->closeDataSource();
   if(SDB_OK != rc2 )
   {
      PD_LOG( PDWARNING, "Failed to close conn pool, rc:%d.", rc2 ) ;
      ossPrintf("Failed to close conn pool(error=%d)."OSS_NEWLINE, rc2);
   }

   if(isCreatePidFile)
   {
      rc2 = ossLockFile ( &pidFile, OSS_LOCK_UN ) ;
      if (SDB_OK != rc2)
      {
         PD_LOG( PDWARNING, "Failed to unlock pid file, rc:%d.", rc2 ) ;
      }
      rc2 = ossClose( pidFile );
      if (SDB_OK != rc2)
      {
         PD_LOG( PDWARNING, "Failed to close pid file, rc:%d.", rc2 ) ;
      }
      rc2 = engine::removePIDFile( pidName ) ;
      if (SDB_OK != rc2)
      {
         PD_LOG( PDWARNING, "Failed to remove pid file, rc:%d.", rc2 ) ;
      }
   } 

done:
   delete sfs;
   return rc;

error:
   goto done;
}



