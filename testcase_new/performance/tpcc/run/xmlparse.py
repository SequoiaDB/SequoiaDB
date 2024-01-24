
try:
   import xml.etree.cElementTree as ET
except ImportError:
   import xml.etree.ElementTree as ET
import sys

class xmlparser:
   def __init__(self, file):
      self.file = file
   def load(self):
      try:
         self.tree = ET.parse(self.file)
         self.root = self.tree.getroot()
         return True
      except Exception, e:
         print e
         return False

   def getRootTag(self):
      return self.root

   def getRootTagProp(self, propName):
      for prop in self.root.attrib:
         if propName == prop:
            return self.root.attrib[prop]
      return ""

   def getAllTag(self, tagName):
      tags = []
      for tag in self.root.findall(tagName):
         tags.append(tag)
      return tags

   def getTagByPos(self, tagName, pos):
      index=0;
      for tag in self.root.findall(tagName):
         if index == pos:
            return tag
         else:
            index = index + 1


   def getAllTagProp(self, tagName, propName):
      props = []
      for tag in self.root.findall(tagName):
         for prop in tag.attrib:
            if propName == prop:
               props.append(tag.attrib[prop])
      return props
  
   def getTagPropByPos(self, tagName, propName, pos):
      index = 0;
      for tag in self.root.findall(tagName):
         if index == pos:
            for prop in tag.attrib:
               if propName == prop:
                  return tag.attrib[prop] 
         else:
            index = index + 1
  
