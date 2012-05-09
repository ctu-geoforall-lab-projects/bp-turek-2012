#!/usr/bin/env python
import sys
import grass.script as grass
import xml.etree.ElementTree as etree 

class Capabilities:

    def __init__(self, cap_file):

        tree = etree.parse(cap_file)
        root_node = tree.getroot()

        self.xml_h = XMLHandlers(root_node) 
       
        if not "version" in root_node.attrib:
            raise CAPError("Missing version attribute root node in Capabilities XML file")
        else:
            self.wms_version = root_node.attrib["version"]

        if self.wms_version == "1.3.0":
            self.proj_tag = "CRS"
        else:
            self.proj_tag = "SRS" 

        cap_node = root_node.find(self.xml_h.ns("Capability"))
        if cap_node is None:
            raise CAPError("Missing <Capability> tag in Capabilities XML file") 

        root_layer_node = cap_node.find(self.xml_h.ns("Layer"))
        if root_layer_node is None:
            raise CAPError("Missing <Layer> tag in Capabilities XML file")  
          
        self.layers_by_id = {}
        self._initializeLayerTree(root_layer_node)

        
    def _initializeLayerTree(self, parent_layer, id = 0):

        if id == 0:
            parent_layer = Layer(parent_layer, None, id, self.xml_h, self.proj_tag)
            self.layers_by_id[id] = parent_layer;
            id += 1

        layer_nodes = parent_layer.layer_node.findall((self.xml_h.ns("Layer")))

        for layer_node in layer_nodes:    
            layer = Layer(layer_node, parent_layer, id, self.xml_h, self.proj_tag)
            self.layers_by_id[id] = layer;
            id += 1
            id = self._initializeLayerTree(layer, id)
            parent_layer.child_layers.append(layer)

        return id;
       

class Layer:

    def __init__(self, layer_node, parent_layer, id, xml_h, proj_tag):

        self.id = id       
        self.xml_h = xml_h
        self.proj_tag = proj_tag
        self.layer_node = layer_node
        self.parent_layer = parent_layer
        self.child_layers = []

        if parent_layer is not None:

            replaced_elements = [ ["EX_GeographicBoundingBox", "replace"], ["Attribution", "replace"], ["MinScaleDenominator", "replace"], 
                                  ["MaxScaleDenominator", "replace"], ["AuthorityURL", "add"]]

            for element in replaced_elements:
                nodes = self.layer_node.findall(self.xml_h.ns(element[0]))

                if len(nodes) != 0 or element[1] == "add":
                    for node in parent_layer.layer_node.findall(self.xml_h.ns(element[0])):
                        self.layer_node.append(node)
 
            inh_arguments = ["queryable", "cascaded", "opaque", "noSubsets", "fixedWidth", "fixedHeight"]

            for attr in parent_layer.layer_node.attrib:
                if attr not in self.layer_node.attrib and attr in inh_arguments:
                    self.layer_node.attrib[attr] = parent_layer.layer_node.attrib[attr]

            self.inhNotSameElement(self.proj_tag, "element_content")
            self.inhNotSameElement("BoundingBox", "attribute", self.proj_tag)
            self.inhNotSameElement("Style", "child_element_content", "Name")
            self.inhNotSameElement("Dimension", "attribute", "name")

    def inhNotSameElement(self, element_name, cmp_type, add_arg = None):
        
        nodes = self.layer_node.findall(self.xml_h.ns(element_name))    

        parent_nodes = []
        if self.parent_layer is not None:
            parent_nodes = self.parent_layer.layer_node.findall(self.xml_h.ns(element_name))

        for parent_node in parent_nodes:

            parent_cmp_text = ""
            if cmp_type == "attribute":
                if add_arg in parent_node.attrib:
                    parent_cmp_text = parent_node.attrib[add_arg];

            elif cmp_type == "element_content":
                parent_cmp_text = parent_node.text

            elif cmp_type == "child_element_content":
                parent_cmp_text = parent_node.find(self.xml_h.ns(add_arg)).text

            if parent_cmp_text == "":
                continue
 
            is_there  = False  
            for node in nodes:

                cmp_text = None
                if cmp_type == "attribute":
                    if add_arg in node.attrib:
                        cmp_text = node.attrib[add_arg];

                elif cmp_type == "element_content":
                         cmp_text = node.text

                elif cmp_type == "child_element_content":
                    cmp_text = node.find(self.xml_h.ns(add_arg)).text

                if cmp_text.lower() == parent_cmp_text.lower():
                    is_there = True
                    break
           
            if  not is_there:
                self.layer_node.append(parent_node)

class XMLHandlers():

    def __init__(self, root_node):
        self.namespace = "{http://www.opengis.net/wms}"

        if root_node.find("Service") is not None:
            self.use_ns = False

        if root_node.find(self.namespace + "Service") is not None:
            self.use_ns = True

        else:
           raise CAPError("Missing <Service> tag in Capability XML file")

    def ns(self, tag_name):

        if self.use_ns:
            tag_name = self.namespace + tag_name

        return tag_name
       
       
class CAPError(Exception):

      def __init__(self, msg):
          self.msg = msg
      def __str__(self):
          return repr(self.msg)

