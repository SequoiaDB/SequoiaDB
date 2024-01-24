import sys
from optparse import OptionParser
from properties import Properties 
from xmlparse import xmlparser 
import subprocess
from subprocess import CalledProcessError

import os
import sys
def getParentPath():
    dir = os.path.abspath(os.path.dirname(sys.argv[0]))
    while "run" not in os.listdir(dir):
        dir = os.path.join(dir, os.path.pardir)
    return os.path.abspath(dir)

parser = OptionParser("usage:%prog [options] ")
parser.add_option("-m", "--mode",
                  action = "store_true",
                  dest="mode",
                  help="Modify the configuration and run it"
                  )
parser.add_option("-s", "--sdburl",
                  action = "store",
                  type='string',
                  dest="sdburl",
                  default= None,
                  help="provide 192.168.30.62:11810,192.168.31.62:11810)"
                  )
parser.add_option("-c", "--conn",
                  action = "store",
                  type='string',
                  dest="conn",
                  default= None,
                  help="provide jdbc usl(jdbc:postgresql://host1:5432/benchmarksql,jdbc:postgresql://host2:5432/benchmarksql)"
                  )
parser.add_option("-u", "--user",
                  action = "store",
                  type='string',
                  dest="user",
                  default= 'benchmarksql',
                  help="database user"
                  )
parser.add_option("-p", "--password",
                  action = "store",
                  type='string',
                  dest="passwd",
                  default= 'changeme',
                  help="database user's password"
                  )
parser.add_option("-w", "--warehouses",
                  action = "store",
                  type='int',
                  dest="warehouses",
                  default= 10,
                  help="warehouses number"
                  )

parser.add_option("-t", "--terminals",
                  action = "store",
                  type='int',
                  dest="terminals",
                  help="concurrent number"
                  )

parser.add_option("-r", "--runMins",
                  action = "store",
                  type='int',
                  dest="runMins",
                  help="run time"
                  )
parser.add_option("-a", "--osCollectorSSHAddr",
                  action = "store",
                  type='string',
                  dest="osCollectorSSHAddr",
                  help="host address(user@host1,user@host2)"
                  )
parser.add_option("-d", "--osCollectorDevices",
                  action = "store",
                  type='string',
                  dest="osCollectorDevices",
                  help="devices(host1:net_em1 blk_sda,host2:net_em1 blk_sda)"
                  )
parser.add_option("-e", "--extension",
                  action = "store_true",
                  default=True,
                  dest="extension",
                  help="test foreign table"
                  )
parser.add_option("-f", "--propfile",
                  action = "store",
                  type='string',
                  dest="propfile",
                  help="provider propfile"
                  )
parser.add_option("-l", "--load",
                  action = "store_true",
                  default=False,
                  dest="load",
                  help="load data"
                  )
(options, args) = parser.parse_args()


if options.mode:
   #if options.sdburl == None:
   #   print("must provider parameter sdburl")
   #   parser.print_help()
   #   sys.exit(1)
   #if options.conn == None:
   #   print("must provider parameter conn")
   #   parser.print_help()
   #   sys.exit(1)
   if options.user == None:
      print("must provider parameter user")   
      parser.print_help()
      sys.exit(1)
   if options.passwd == None:
      print("must provider parameter passwd")
      parser.print_help()
      sys.exit(1)
   if options.warehouses == None:
      print("must provider parameter warehouses")
      parser.print_help()
      sys.exit(1)
   if options.terminals == None:
      print("must provider terminals ") 
      parser.print_help()
      sys.exit(1)
   if options.runMins == None:
      print("must provider runMins ") 
      parser.print_help()
      sys.exit(1)
   #if options.osCollectorSSHAddr == None:
   #   print("must provider osCollectorSSHAddr ") 
   #   parser.print_help()
   #   sys.exit(1)
   #if options.osCollectorDevices == None:
   #   print("must provider osCollectorDevices ") 
   #   sys.exit(1)
   if options.propfile == None:
      print("must provider propfile ")
      sys.exit(1)
   try:
      prop = Properties(options.propfile) 
      prop.load()
      if options.sdburl != None:
         prop.setProp('sdburl', options.sdburl)
      if options.conn != None:
         prop.setProp('conn', options.conn)
      prop.setProp('user', options.user)
      prop.setProp('password', options.passwd)
      prop.setProp('warehouses', options.warehouses)
      prop.setProp('terminals', options.terminals)
      prop.setProp('runMins', options.runMins)
      prop.setProp('load', options.load)
      if options.osCollectorSSHAddr != None:
         prop.setProp('osCollectorSSHAddr', options.osCollectorSSHAddr)
      if options.osCollectorDevices != None: 
         prop.setProp('osCollectorDevices', options.osCollectorDevices)
      if options.extension:
         prop.setProp('testType', 'fdw')
      prop.save()
   except Exception,e:
      print e

xml = xmlparser('tpc_c.xml')   
if not xml.load() :
   print "parse %s failed"%('tpc_c.xml')
   sys.exit(1)
   
steps = xml.getAllTag('step')
for step in steps:
   notexec = False
   for prop in step.attrib:
      if prop == 'exec' and step.attrib[prop] != '1':
         notexec = True
         break
   if notexec:
      continue
   del step.attrib['exec']
   curpath = getParentPath()
   try:
      retcode = subprocess.check_call(['bash', curpath+"/run/"+step.text, curpath+"/run/"+step.attrib['parameter']])
   except CalledProcessError,e:
      print e.returncode
      sys.exit(e.returncode)

   
   
