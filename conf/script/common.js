/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: Js class for the js files in current document
@modify list:
   2014-7-26 Zhaobo Tan  Init
*/

function commonResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
}

function scanHostResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.Status                    = "" ;
   this.IP                        = "" ;
   this.HostName                  = "" ;
}

function preCheckResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.IP                        = "" ;
   this.AgentService              = "" ;
}

function postCheckResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.IP                        = "" ;
}

function addHostCheckEnvResult()
{
   this.MD5                       = "" ;
   this.SDBADMIN_USER             = "" ;
   this.INSTALL_DIR               = "" ;
   this.OMA_SERVICE               = "" ;
   this.ISPROGRAMEXIST            = false ;
}

function addHostResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.IP                        = "" ;
   this.version                   = "" ;
   //this.HasInstall                = false ;
}

function removeHostResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.IP                        = "" ;
   //this.HasUninstall              = false ;
}

function addHostCheckInfoResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
}

function addHostRollbackResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.IP                        = "" ;
   this.HasUninstall              = false ;
}

function installNodeResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
}

function removeNodeResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
}

function removeRGResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
}

function rollbackNodeResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
}

function installTmpCoordResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.TmpCoordSvcName           = "" ;
}

function removeTmpCoordResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
}

function tmpCoordOption()
{
   this.type                      = "db" ;
   this.role                      = "coord" ;
   this.mode                      = "run" ;
   this.expand                    = true ;
}

function tmpCoordMather()
{
   this.clustername               = "" ;
   this.businessname              = "" ;
   this.usertag                   = "" ;
}

function checkSAInfo()
{
   this.clustername               = "" ;
   this.businessname              = "" ;
   this.usertag                   = "" ;
   this.svcname                   = "" ;
}

function psqlResult()
{
   this.errno                    = SDB_OK ;
   this.detail                    = "" ; 
   this.PipeFile                  = "" ;
}

function cleanPsqlResult()
{
   this.errno                    = SDB_OK ;
   this.detail                    = "" ; 
}

