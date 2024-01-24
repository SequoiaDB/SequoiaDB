if ( tmpFile == undefined )
{
   var tmpFile = {
      _close: File.prototype._close,
      _getInfo: File.prototype._getInfo,
      _getPermission: File.prototype._getPermission,
      _read: File.prototype._read,
      _readContent: File.prototype._readContent,
      _readLine: File.prototype._readLine,
      _seek: File.prototype._seek,
      _toString: File.prototype._toString,
      _truncate: File.prototype._truncate,
      _write: File.prototype._write,
      _writeContent: File.prototype._writeContent,
      chgrp: File.prototype.chgrp,
      chmod: File.prototype.chmod,
      chown: File.prototype.chown,
      close: File.prototype.close,
      copy: File.prototype.copy,
      exist: File.prototype.exist,
      find: File.prototype.find,
      getInfo: File.prototype.getInfo,
      getSize: File.prototype.getSize,
      getUmask: File.prototype.getUmask,
      help: File.prototype.help,
      isDir: File.prototype.isDir,
      isEmptyDir: File.prototype.isEmptyDir,
      isFile: File.prototype.isFile,
      list: File.prototype.list,
      md5: File.prototype.md5,
      mkdir: File.prototype.mkdir,
      move: File.prototype.move,
      read: File.prototype.read,
      readContent: File.prototype.readContent,
      readLine: File.prototype.readLine,
      remove: File.prototype.remove,
      seek: File.prototype.seek,
      setUmask: File.prototype.setUmask,
      stat: File.prototype.stat,
      toString: File.prototype.toString,
      truncate: File.prototype.truncate,
      write: File.prototype.write,
      writeContent: File.prototype.writeContent
   };
}
var funcFile = ( funcFile == undefined ) ? File : funcFile;
var funcFile_find = funcFile._find;
var funcFile_getFileObj = funcFile._getFileObj;
var funcFile_getPathType = funcFile._getPathType;
var funcFile_getPermission = funcFile._getPermission;
var funcFile_getUmask = funcFile._getUmask;
var funcFile_list = funcFile._list;
var funcFile_readFile = funcFile._readFile;
var funcFilechgrp = funcFile.chgrp;
var funcFilechmod = funcFile.chmod;
var funcFilechown = funcFile.chown;
var funcFilecopy = funcFile.copy;
var funcFileexist = funcFile.exist;
var funcFilefind = funcFile.find;
var funcFilegetSize = funcFile.getSize;
var funcFilegetUmask = funcFile.getUmask;
var funcFilehelp = funcFile.help;
var funcFileisDir = funcFile.isDir;
var funcFileisEmptyDir = funcFile.isEmptyDir;
var funcFileisFile = funcFile.isFile;
var funcFilelist = funcFile.list;
var funcFilemd5 = funcFile.md5;
var funcFilemkdir = funcFile.mkdir;
var funcFilemove = funcFile.move;
var funcFileremove = funcFile.remove;
var funcFilescp = funcFile.scp;
var funcFilesetUmask = funcFile.setUmask;
var funcFilestat = funcFile.stat;
File=function(){try{return funcFile.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File._find = function(){try{ return funcFile_find.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File._getFileObj = function(){try{ return funcFile_getFileObj.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File._getPathType = function(){try{ return funcFile_getPathType.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File._getPermission = function(){try{ return funcFile_getPermission.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File._getUmask = function(){try{ return funcFile_getUmask.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File._list = function(){try{ return funcFile_list.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File._readFile = function(){try{ return funcFile_readFile.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.chgrp = function(){try{ return funcFilechgrp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.chmod = function(){try{ return funcFilechmod.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.chown = function(){try{ return funcFilechown.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.copy = function(){try{ return funcFilecopy.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.exist = function(){try{ return funcFileexist.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.find = function(){try{ return funcFilefind.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.getSize = function(){try{ return funcFilegetSize.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.getUmask = function(){try{ return funcFilegetUmask.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.help = function(){try{ return funcFilehelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.isDir = function(){try{ return funcFileisDir.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.isEmptyDir = function(){try{ return funcFileisEmptyDir.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.isFile = function(){try{ return funcFileisFile.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.list = function(){try{ return funcFilelist.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.md5 = function(){try{ return funcFilemd5.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.mkdir = function(){try{ return funcFilemkdir.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.move = function(){try{ return funcFilemove.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.remove = function(){try{ return funcFileremove.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.scp = function(){try{ return funcFilescp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.setUmask = function(){try{ return funcFilesetUmask.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.stat = function(){try{ return funcFilestat.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
File.prototype._close=function(){try{return tmpFile._close.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._getInfo=function(){try{return tmpFile._getInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._getPermission=function(){try{return tmpFile._getPermission.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._read=function(){try{return tmpFile._read.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._readContent=function(){try{return tmpFile._readContent.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._readLine=function(){try{return tmpFile._readLine.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._seek=function(){try{return tmpFile._seek.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._toString=function(){try{return tmpFile._toString.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._truncate=function(){try{return tmpFile._truncate.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._write=function(){try{return tmpFile._write.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype._writeContent=function(){try{return tmpFile._writeContent.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.chgrp=function(){try{return tmpFile.chgrp.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.chmod=function(){try{return tmpFile.chmod.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.chown=function(){try{return tmpFile.chown.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.close=function(){try{return tmpFile.close.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.copy=function(){try{return tmpFile.copy.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.exist=function(){try{return tmpFile.exist.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.find=function(){try{return tmpFile.find.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.getInfo=function(){try{return tmpFile.getInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.getSize=function(){try{return tmpFile.getSize.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.getUmask=function(){try{return tmpFile.getUmask.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.help=function(){try{return tmpFile.help.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.isDir=function(){try{return tmpFile.isDir.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.isEmptyDir=function(){try{return tmpFile.isEmptyDir.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.isFile=function(){try{return tmpFile.isFile.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.list=function(){try{return tmpFile.list.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.md5=function(){try{return tmpFile.md5.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.mkdir=function(){try{return tmpFile.mkdir.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.move=function(){try{return tmpFile.move.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.read=function(){try{return tmpFile.read.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.readContent=function(){try{return tmpFile.readContent.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.readLine=function(){try{return tmpFile.readLine.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.remove=function(){try{return tmpFile.remove.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.seek=function(){try{return tmpFile.seek.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.setUmask=function(){try{return tmpFile.setUmask.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.stat=function(){try{return tmpFile.stat.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.toString=function(){try{return tmpFile.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.truncate=function(){try{return tmpFile.truncate.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.write=function(){try{return tmpFile.write.apply(this,arguments);}catch(e){throw new Error(e);}};
File.prototype.writeContent=function(){try{return tmpFile.writeContent.apply(this,arguments);}catch(e){throw new Error(e);}};
