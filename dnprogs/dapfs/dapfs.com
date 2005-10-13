$!-----------------------------------------------------------------------------
$!     (C) Eduardo Marcelo Serrat              <emserrat@geocities.com>
$!     dapfs DECnet OBJECT
$!
$!     It is installed as a DECnet object to cooperate with
$!     Linux dapfs filesystem implementation to create directories
$!     and delete them.
$!
$!
$!     Installation on VAX/OpenVMS DECnet PHASE IV:
$!      -------------------------------------------
$!     1) copy dapfs.com to SYS$SYSTEM:
$!
$!     2) define the dapfs object in the DECnet database
$!
$!     $ MCR NCP DEFINE OBJECT DAPFS NUMBER 0 FILE SYS$SYSTEM:LINUXDIR.COM
$!     $ MCR NCP SET OBJECT DAPFS ALL
$!
$!      Installation on Alpha/OpenVMS DECnet-Plus or VAX/DECnet OSI
$!
$!     1) copy dapfs.com to SYS$SYSTEM
$!
$!     2) define the dapfs object in the DECnet database
$!
$!     $ mcr ncl create node 0 session control application dapfs
$!     $ mcr ncl set node 0 session control application dapfs -
$!        addresses = {name=dapfs}, image name = sys$system:dapfs.com
$!
$!     3) edit and include preceding commands into:
$!             SYS$MANAGER:NET$STARTUP_APPLICATIONS.NCL 
$!
$!----------------------------------------------------------------------------
$ if f$mode() .nes. "NETWORK" then exit
$!
$ open/read/write dapfs        sys$net
$ read/prompt=""/time_out=5/error=out dapfs operation
$ read/prompt=""/time_out=5/error=out dapfs dirname
$!
$ if operation .eqs. "CREATE" then $goto create_op
$ if operation .eqs. "STATFS" then $goto statfs_op
$ set file/prot=(o:rwed) 'dirname'
$ delete/nolog 'dirname'
$ if $severity .ne. 1 then $goto del_error
$ write dapfs "OK"
$ goto out
$del_error:
$ set file/prot=(o:rwe) 'dirname' 
$ goto dir_error
$!
$create_op:
$ create/directory 'dirname'
$ if $severity .ne. 1 then $goto dir_error
$ write dapfs "OK"
$ goto out
$!
$statfs_op:
$! TODO PJC
$dir_error:
$ write dapfs "ERROR"
$!
$out:
$ close/nolog dapfs
$ exit  

