#!/usr/bin/env python
"""
MODULE:    r.in.wms2

AUTHOR(S): Stepan Turek <stepan.turek AT seznam.cz>

PURPOSE:   Downloads and imports data from WMS server.

COPYRIGHT: (C) 2012 Stepan Turek, and by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.
"""
import sys

import grass.script as grass
import xml.dom.minidom as xml

import codecs

class Capabilities:
    def __init__(self, cap_string):
        cap_string = cap_string.encode('utf-8')
        self.xml_h = XMLHandlers();##TODO parse inside cap_string
        self.xml_cap = xml.parseString(cap_string)
        #self.xml_cap = xml.parse("/home/ostepok/Desktop/xml.xml")
        root_node = self.xml_cap.documentElement
        #self._cleanTree(root_node)
        self.wms_version = self.xml_h.getAttr(root_node, "version")##TODO upper case...

        if self.wms_version == "1.3.0":
            self.proj_tag = "CRS"
        else:
            self.proj_tag = "SRS" 

        error_suffix = "in GetCapabilities response";

        service_node = self.xml_h.childMustExist(root_node ,"Service", error_suffix)
        cap_node = self.xml_h.childMustExist(root_node , "Capability", error_suffix)
        root_layer_node = self.xml_h.childMustExist(cap_node ,"Layer", error_suffix)


        self.layers_by_id = {}
        self._initializeLayerTree(root_layer_node)
        
    def _initializeLayerTree(self, parent_layer, id = 0):

        if id == 0:
            parent_layer = self._initializeLayer(parent_layer, id)
            id += 1

        layer_nodes =  self.xml_h.getChildren(parent_layer._layer_node, "Layer")

        for layer_node in layer_nodes:    
            layer = self._initializeLayer(layer_node, id, parent_layer)
            id += 1
            id = self._initializeLayerTree(layer, id)
            parent_layer.child_layers.append(layer)

        return id;
        
    def _initializeLayer(self, layer_node, id, parent_layer = None):

        layer = Layer(layer_node, parent_layer, id, self.xml_h, self.proj_tag)
        self.layers_by_id[id] = layer;

        return layer
       

class Layer:

    def __init__(self, layer_node, parent_layer, id, xml_h, proj_tag):

        self.id = id       
        self.xml_h = xml_h
        self.proj_tag = proj_tag
        self._layer_node = layer_node
        self.parent_layer = parent_layer
        self.child_layers = []

        self._inherited_elements = {}
        self._inherited_elements["style_nodes"] = [] 
        self._inherited_elements["bbox_nodes"] = []
        self._inherited_elements["proj_nodes"] = [] 

        self.inheritElement(parent_layer, "proj_nodes", proj_tag, "element_content")
        self.inheritElement(parent_layer, "bbox_nodes", "BoundingBox", "attribute", self.proj_tag)
        self.inheritElement(parent_layer, "style_nodes", "Style", "child_element_content", "Name")
     
        #grass.message("Layer:" + str(id))
        #grass.message(len(self._inherited_elements["proj_nodes"]))
        #grass.message(len(self._inherited_elements["bbox_nodes"]))
        #grass.message(len(self._inherited_elements["style_nodes"]))

    def inheritElement(self, parent_layer, inherited_element, element_name, cmp_type, add_arg = None):#TODO inheritElements??? 
        
        nodes = self.xml_h.getChildren(self._layer_node, element_name)    
        self._inherited_elements[inherited_element] = nodes

        parent_nodes = []
        if parent_layer is not None:
            parent_nodes = parent_layer._inherited_elements[inherited_element]

        for parent_node in parent_nodes:

            parent_cmp_text = ""
            if cmp_type == "attribute":
                parent_cmp_text = self.xml_h.getAttr(parent_node, add_arg)
 
            elif cmp_type == "element_content":
                parent_cmp_text = self.xml_h.getNodeContent(parent_node)

            elif cmp_type == "child_element_content":
                parent_cmp_node = self.xml_h.getChild(parent_node, add_arg)
                if parent_cmp_node:
                    parent_cmp_text = self.xml_h.getNodeContent(parent_cmp_node)
                else:
                    continue;

            if parent_cmp_text == "":
                continue
 
            is_there  = False  
            for node in nodes:

                cmp_text = None
                if cmp_type == "attribute":
                    cmp_text = self.xml_h.getAttr(node, add_arg)

                elif cmp_type == "element_content":
                    cmp_text = self.xml_h.getNodeContent(node)

                elif cmp_type == "child_element_content":
                    cmp_node = self.xml_h.getChild(node, add_arg)
                    if parent_node:
                        cmp_text = self.xml_h.getNodeContent(cmp_node)
                    else: 
                        continue

                if cmp_text.lower() == parent_cmp_text.lower():
                    is_there = True
                    break
           
            if  not is_there:
                self._inherited_elements[inherited_element].append(parent_node)

    def getName(self):
        return self.xml_h.getChildContent(self._layer_node, "Name")
        
    def getTitle(self):
        return self.xml_h.getChildContent(self._layer_node, "Title") 

    def getStyles(self):

        style_nodes = self.xml_h.getChildren(self._layer_node, "Style")
        styles = []

        for style_node in style_nodes:
            styles.append(Style(style_node, self.xml_h))

        return styles

    #def getProj(self, proj):##TODO
    #    proj_node = self.xml_h.getChild(self._layer_node, self.proj_tag + ":" + proj);        
    #    return self.xml_h.getNodeContent(proj_node);

class Style:##BASE CLASS

    def __init__(self, style_node, xml_h):
        self.xml_h = xml_h;
        self._style_node = style_node;

    def getName(self):
        return self.xml_h.getChildContent(self._style_node, "Name") 
        
    def getTitle(self):
        return self.xml_h.getChildContent(self._style_node, "Title") 
        
class XMLHandlers:

    def getChildren(self, parent_node, name):

        nodes = parent_node.childNodes
        found_nodes = []
        if parent_node is not None:

            for node in nodes:

                if not node.localName:
                    continue;

	        if node.localName.lower() == name.lower() and node.nodeType == xml.Node.ELEMENT_NODE:
                     found_nodes.append(node)
         
        return  found_nodes;

    def getChild(self, parent_node, name):

        nodes = parent_node.childNodes
        if parent_node is not None:

            for node in nodes:

                if not node.localName:
                    continue;

	        if node.localName.lower() == name.lower() and node.nodeType == xml.Node.ELEMENT_NODE:
                    return node; 
         
        return  None;

    def getChildContent(self, parent_node, name):

        name_node = self.getChild(parent_node, name)
        if name_node:
            return self.getNodeContent(name_node)
        else:
            return None

    def getAttr(self, node, name):

        if node.hasAttribute(name):
            return node.getAttribute(name)   
        
        return  None;

    def getChildByAttr(self, node, attrName):

        nodes = getChildren(self, node, name)
 
        for node in nodes:

            if node.hasAttribute(name):
                return node  
 
        return  None;


    def getNodeContent(self, parrent_node):
        text_list = []
        nodelist = parrent_node.childNodes

        for node in nodelist:

            if node.nodeType == xml.Node.TEXT_NODE:
                text_list.append(node.wholeText)
  
        if len(text_list) > 0:
            return ' '.join(text_list)

        else:
            return ""

    def childMustExist(self, parent_node, tag_name, error_suffix):

       cap_node = self.getChildren(parent_node, tag_name)

       if len(cap_node) == 0:##TODO only one
           raise CAPError("Missing <" + tag_name + "> tag in" + error_suffix)
       return cap_node[0]

class CAPError(Exception):

      def __init__(self, msg):
          self.msg = msg
      def __str__(self):
          return repr(self.msg)

