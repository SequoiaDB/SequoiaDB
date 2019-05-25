/*                                                                -*- C -*-
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2007 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Stig Sæther Bakken <ssb@php.net>                             |
   +----------------------------------------------------------------------+
*/

/* $Id: build-defs.h.in 310307 2011-04-18 09:50:32Z bjori $ */

#define CONFIGURE_COMMAND " '../configure'  '--prefix=/usr' '--with-apxs2=/usr/bin/apxs2' '--with-config-file-path=/etc/php5/apache2' '--with-config-file-scan-dir=/etc/php5/apache2/conf.d' '--build=x86_64-linux-gnu' '--host=x86_64-linux-gnu' '--sysconfdir=/etc' '--localstatedir=/var' '--mandir=/usr/share/man' '--disable-debug' '--with-regex=php' '--disable-rpath' '--disable-static' '--with-pic' '--with-layout=GNU' '--with-pear=/usr/share/php' '--enable-calendar' '--enable-sysvsem' '--enable-sysvshm' '--enable-sysvmsg' '--enable-bcmath' '--with-bz2' '--enable-ctype' '--with-db4' '--without-gdbm' '--with-iconv' '--enable-exif' '--enable-ftp' '--with-gettext' '--enable-mbstring' '--with-pcre-regex=/usr' '--enable-shmop' '--enable-sockets' '--enable-wddx' '--with-libxml-dir=/usr' '--with-zlib' '--with-kerberos=/usr' '--with-openssl' '--enable-soap' '--enable-zip' '--with-mhash=yes' '--with-exec-dir=/usr/lib/php5/libexec' '--with-system-tzdata' '--with-mysql-sock=/var/run/mysqld/mysqld.sock' '--without-mm' '--with-curl=shared,/usr' '--with-enchant=shared,/usr' '--with-zlib-dir=/usr' '--with-gd=shared,/usr' '--enable-gd-native-ttf' '--with-gmp=shared,/usr' '--with-jpeg-dir=shared,/usr' '--with-xpm-dir=shared,/usr/X11R6' '--with-png-dir=shared,/usr' '--with-freetype-dir=shared,/usr' '--enable-intl=shared' '--with-ttf=shared,/usr' '--with-t1lib=shared,/usr' '--with-ldap=shared,/usr' '--with-ldap-sasl=/usr' '--with-mysql=shared,/usr' '--with-mysqli=shared,/usr/bin/mysql_config' '--with-pspell=shared,/usr' '--with-unixODBC=shared,/usr' '--with-recode=shared,/usr' '--with-xsl=shared,/usr' '--with-snmp=shared,/usr' '--without-sqlite' '--with-sqlite3=shared,/usr' '--with-mssql=shared,/usr' '--with-tidy=shared,/usr' '--with-xmlrpc=shared' '--with-pgsql=shared,/usr'"
#define PHP_ADA_INCLUDE		""
#define PHP_ADA_LFLAGS		""
#define PHP_ADA_LIBS		""
#define PHP_APACHE_INCLUDE	""
#define PHP_APACHE_TARGET	""
#define PHP_FHTTPD_INCLUDE      ""
#define PHP_FHTTPD_LIB          ""
#define PHP_FHTTPD_TARGET       ""
#define PHP_CFLAGS		"$(CFLAGS_CLEAN) -prefer-pic"
#define PHP_DBASE_LIB		""
#define PHP_BUILD_DEBUG		""
#define PHP_GDBM_INCLUDE	""
#define PHP_IBASE_INCLUDE	""
#define PHP_IBASE_LFLAGS	""
#define PHP_IBASE_LIBS		""
#define PHP_IFX_INCLUDE		""
#define PHP_IFX_LFLAGS		""
#define PHP_IFX_LIBS		""
#define PHP_INSTALL_IT		"$(mkinstalldirs) '$(INSTALL_ROOT)/usr/lib/apache2/modules' &&                 $(mkinstalldirs) '$(INSTALL_ROOT)/etc/apache2' &&                  /usr/bin/apxs2 -S LIBEXECDIR='$(INSTALL_ROOT)/usr/lib/apache2/modules'                        -S SYSCONFDIR='$(INSTALL_ROOT)/etc/apache2'                        -i -a -n php5 libphp5.la"
#define PHP_IODBC_INCLUDE	""
#define PHP_IODBC_LFLAGS	""
#define PHP_IODBC_LIBS		""
#define PHP_MSQL_INCLUDE	""
#define PHP_MSQL_LFLAGS		""
#define PHP_MSQL_LIBS		""
#define PHP_MYSQL_INCLUDE	"-I/usr/include/mysql"
#define PHP_MYSQL_LIBS		"-L/usr/lib/x86_64-linux-gnu -lmysqlclient_r "
#define PHP_MYSQL_TYPE		"external"
#define PHP_ODBC_INCLUDE	"-I/usr/include"
#define PHP_ODBC_LFLAGS		"-L/usr/lib"
#define PHP_ODBC_LIBS		"-lodbc"
#define PHP_ODBC_TYPE		"unixODBC"
#define PHP_OCI8_SHARED_LIBADD 	""
#define PHP_OCI8_DIR			""
#define PHP_OCI8_ORACLE_VERSION		""
#define PHP_ORACLE_SHARED_LIBADD 	"@ORACLE_SHARED_LIBADD@"
#define PHP_ORACLE_DIR				"@ORACLE_DIR@"
#define PHP_ORACLE_VERSION			"@ORACLE_VERSION@"
#define PHP_PGSQL_INCLUDE	""
#define PHP_PGSQL_LFLAGS	""
#define PHP_PGSQL_LIBS		""
#define PHP_PROG_SENDMAIL	"/usr/sbin/sendmail"
#define PHP_SOLID_INCLUDE	""
#define PHP_SOLID_LIBS		""
#define PHP_EMPRESS_INCLUDE	""
#define PHP_EMPRESS_LIBS	""
#define PHP_SYBASE_INCLUDE	""
#define PHP_SYBASE_LFLAGS	""
#define PHP_SYBASE_LIBS		""
#define PHP_DBM_TYPE		""
#define PHP_DBM_LIB		""
#define PHP_LDAP_LFLAGS		""
#define PHP_LDAP_INCLUDE	""
#define PHP_LDAP_LIBS		""
#define PHP_BIRDSTEP_INCLUDE     ""
#define PHP_BIRDSTEP_LIBS        ""
#define PEAR_INSTALLDIR         "/usr/share/php"
#define PHP_INCLUDE_PATH	".:/usr/share/php:/usr/share/pear"
#define PHP_EXTENSION_DIR       "/usr/lib/php5/20090626"
#define PHP_PREFIX              "/usr"
#define PHP_BINDIR              "/usr/bin"
#define PHP_SBINDIR             "/usr/sbin"
#define PHP_MANDIR              "/usr/share/man"
#define PHP_LIBDIR              "/usr/lib/php5"
#define PHP_DATADIR             "${prefix}/share"
#define PHP_SYSCONFDIR          "/etc"
#define PHP_LOCALSTATEDIR       "/var"
#define PHP_CONFIG_FILE_PATH    "/etc/php5/apache2"
#define PHP_CONFIG_FILE_SCAN_DIR    "/etc/php5/apache2/conf.d"
#define PHP_SHLIB_SUFFIX        "so"
