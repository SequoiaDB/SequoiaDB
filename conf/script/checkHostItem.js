/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

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
@description: the following items are need to check by checkHost.js
@modify list:
   2014-7-26 Zhaobo Tan  Init
*/

function OSInfo()
{
   this.Distributor        = "" ;
   this.Release            = "" ;
   this.Description        = "" ;
   this.Bit                = 0 ;
}

function OMAInfo()
{
   //this.HasInstalled       = false ;
   this.Version            = "" ;
   this.SdbUser            = "" ;
   this.Path               = "" ;
   this.Service            = "" ;
   this.Release            = "" ;
}

function MemoryInfo()
{
   this.Model              = "" ;
   this.Size               = 0 ;
   this.Free               = 0 ;
}

function DiskInfo()
{
   this.Name               = "" ;
   this.Mount              = "" ;
   this.Size               = 0 ;
   this.Free               = 0 ;
   this.IsLocal            = false ;
}

function CPUInfo()
{
   this.ID                 = "" ;
   this.Model              = "" ;
   this.Core               = "" ;
   this.Freq               = "" ; 
}

function NetInfo()
{
   this.Name               = "" ;
   this.Model              = "" ;
   this.Bandwidth          = "" ;
   this.IP                 = "" ;
}

function PortInfo()
{
   this.Port               = "" ;
   this.CanUse             = false ;
}

function ServiceInfo()
{
   this.Name               = "" ;
   this.IsRunning          = false ;
   this.Version            = "" ;
}

function SafetyInfo()
{
   this.Name               = "" ;
   this.Context            = "" ;
   this.IsRunning          = false ;
}
