import os
class Properties(object):
   def __init__(self, fileName):
      self.fileName = fileName
      self.properties = {}
   def __getDict(self, strName, dictName, value):
      if ( strName.find( '.' ) > 0 ):
         k = strName.split( '.' )[0]
         dictName.setdefault( k, {} )
         return self.__getDict( strName[len(k+1):], dictName[k], value )
      else:
         dictName[str(strName)] = str(value)
         return
    
   def load(self):
      try:
         pro_file = open(self.fileName, "Ur")
         for line in pro_file.readlines():
            line = line.strip().replace('\n', '')
            pos = line.find('//')
            if pos != -1 and pos == 0:
              continue
            if line.find('#') != -1:
               line = line[0:line.find('#')]
            if line.find('=') > 0 : 
               strs = line.split('=')
               strs[1] = line[len(strs[0])+1:]
               self.__getDict(strs[0].strip(), self.properties, strs[1].strip())
      except Exception, e:
         raise e
      finally:
         pro_file.close()
   
   def getProp(self, name):
      if ( False == self.properties.has_key( name ) ):
         return ""
      else:
         return self.properties[name]

   def setProp(self, name, val):
      self.properties[str(name)] = str(val)
    
   def save(self):
      try:
         #if os.access(self.fileName, F_OK):
         #   os.rename(self.fileName, self.fileName + ".bak")
         pro_file_new = open(self.fileName, "w")
         #pro_file_old = open(self.fileName + ".bak", "Ur")
         #for line in pro_file_old.readlines():
         #   if line.find('//') == 0:
         #      pro_file_new.write(line)
         #   elif line.find('=') > 0:
         #      strs = line.split('=')
         #      val = self.getProp(strs[0].strip())
         #      if ( type(val) is str ) and ( len( val ) == 0 ):
         #         pro_file_new.write(line)
         #      else:
         #         pro_file_new.write(strs[0].strip() + '=' + str(val) + "\n")
         for key in self.properties:
            val = self.properties[key]
            pro_file_new.write(key + '=' + val + "\n")
      except Exception, e:
         raise e
      finally:
         #pro_file_old.close() 
         pro_file_new.close()
