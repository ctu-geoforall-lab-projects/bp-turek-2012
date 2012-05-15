"""!
@package ogc.services.wms

@brief Dialogs for OGC WMS

List of classes:
 - wms::WMSDialog
 - wms::WMSCapabilities
 - wms::WMSLayer

(C) 2009-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Stepan Turek <stepan.turek seznam.cz>
"""

import xml.etree.ElementTree as etree 

import wx

from core.gcmd         import RunCommand, GException, GError
from core.settings     import UserSettings
from ogc_services.base import LayersList, OGCDialog, XMLHandlers

import grass.script as grass

class WMSDialog(OGCDialog):
    def __init__(self, parent, **kwargs):
        """!Dialog for importing data from WMS server"""
        OGCDialog.__init__(self, parent, **kwargs)
        
        self.SetTitle(_("Import data from WMS server"))
        
        self.SetMinSize((550, 400))
        
    def _createWidgets(self):
        """!Create dialog widgets"""
        self.settingsBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                        label = _(" Server settings "))
        
        self.serverText = wx.StaticText(parent = self.panel, id = wx.ID_ANY, label = _("Server:"))
        self.server  = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY)
        
        # list of layers
        self.layersBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                      label=_(" List of layers "))

        self.list = LayersList(self.panel)
        self.list.LoadData()
        
        self.infoPanel  = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY, style = wx.TE_MULTILINE)
        
        self.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnTreeItemClicked, self.list)
        
        self.add = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                               label = _("Add imported layers into layer tree"))
        self.add.SetValue(UserSettings.Get(group  ='cmd', key = 'addNewLayer', subkey  ='enabled'))
        
        # buttons
        self.btn_cancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btn_cancel.SetToolTipString(_("Close dialog"))
        self.btn_connect = wx.Button(parent = self.panel, id = wx.ID_ANY, label = _("&Connect"))
        self.btn_connect.SetToolTipString(_("Connect to the server"))
        self.btn_connect.SetDefault()
        if not self.server.GetValue():
            self.btn_connect.Enable(False)
        self.btn_import = wx.Button(parent = self.panel, id = wx.ID_OK, label = _("&Import"))
        self.btn_import.SetToolTipString(_("Import selected layers"))
        self.btn_import.Enable(False)
        
        # bindings
        self.btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        self.btn_connect.Bind(wx.EVT_BUTTON, self.OnConnect)
        self.server.Bind(wx.EVT_TEXT, self.OnServer)
        
    def _layout(self):
        """!Do dialog layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        
        # settings
        settingsSizer = wx.StaticBoxSizer(self.settingsBox, wx.HORIZONTAL)
        
        gridSizer = wx.FlexGridSizer(cols = 2, vgap = 5, hgap = 5)

        gridSizer.Add(item = self.serverText,
                      flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.AddGrowableCol(1)
        gridSizer.Add(item = self.server,
                      flag = wx.EXPAND | wx.ALL)
        
        settingsSizer.Add(item = gridSizer, proportion = 1,
                       flag = wx.EXPAND | wx.ALL)
        
        dialogSizer.Add(item = settingsSizer, proportion = 0,
                        flag = wx.ALL | wx.EXPAND, border = 5)
        
        # list of layers
        layersSizer = wx.StaticBoxSizer(self.layersBox, wx.HORIZONTAL)

        layersSizer.Add(item = self.list, proportion = 1,
                        flag = wx.ALL | wx.EXPAND, border = 5)

        layersSizer.Add(self.infoPanel, proportion = 2,
                        flag = wx.ALL | wx.EXPAND, border = 5)
        
        dialogSizer.Add(item = layersSizer, proportion = 1,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)

        dialogSizer.Add(item = self.add, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        # buttons
        btnsizer = wx.BoxSizer(orient = wx.HORIZONTAL)

        btnsizer.Add(item = self.btn_cancel, proportion = 0,
                     flag = wx.ALL | wx.ALIGN_CENTER,
                     border = 10)
        
        btnsizer.Add(item = self.btn_connect, proportion = 0,
                     flag = wx.ALL | wx.ALIGN_CENTER,
                     border = 10)
        
        btnsizer.Add(item = self.btn_import, proportion = 0,
                     flag = wx.ALL | wx.ALIGN_CENTER,
                     border = 10)
        
        dialogSizer.Add(item = btnsizer, proportion = 0,
                        flag = wx.ALIGN_CENTER)
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)
        self.Layout()
   
    
    def OnCancel(self, event):
        """!Close the dialog"""
        self.Close()

    def OnConnect(self, event):
        """!Connect to the server"""
        server = self.server.GetValue()
        if not server:
            self.btn_import.Enable(False)
            return # not reachable
        
        layers = {}
        ret = RunCommand('r.in.wms2',
                         quiet = True,
                         parent = self,
                         wms_version = "1.3.0",
                         read = True,
                         flags = 'c',
                         mapserver = server)
        
        temp_file = grass.tempfile()
        if temp_file is None:
            grass.fatal(_("Unable to create temporary files"))##TOFO
        
        temp = open(temp_file, "w")
        temp.write(ret.encode('utf-8'))#TODO
        temp.close()
        
        if not ret:
            self.list.LoadData()
            self.btn_import.Enable(False)
            return # no layers found
        
        try:
           self.cap = WMSCapabilities(temp_file)
        except GException as error:
            GError(error, parent = self)
        
        # update list of layers
        self.list.LoadData(self.cap)
        
        if len(layers.keys()) > 0:
            self.btn_import.Enable(True)
        else:
            self.btn_import.Enable(False)
        
    def OnServer(self, event):
        """!Server settings changed"""
        value = event.GetString()
        if value:
            self.btn_connect.Enable(True)
        else:
            self.btn_connect.Enable(False)
        
    def OnTreeItemClicked(self, event): 
        data = [  ["Name", "Name", "text"],
                  ["Title", "Title", "text"],
                  ["Abstract", "Abstract", "text"],  
                  ["queryable", "Queryable", "attribute"]  ]
        
        infoText = ""
        for dataItem in data:
            layer = self.list.GetItemPyData(event.GetItem())
            element =layer.xml_h.ns(dataItem[0])
            elemText = ""
            
            if dataItem[2] == "text":
                if layer.layer_node.find(element) is not None:
                    elemText = layer.layer_node.find(element).text 
            
            elif dataItem[2] == "attribute":
                if layer.layer_node.find(element) is not None:
                    elemText = layer.layer_node.attrib[element];
            
            infoText += dataItem[1] + ": " +  elemText + "\n \n"; 
        
        self.infoPanel.SetValue(infoText)
        
    def GetLayers(self):
        """!Get list of selected layers/styles to be imported"""
        return self.list.GetSelectedLayers()

    def GetSettings(self):
        """!Get connection settings"""
        return { 'server' : self.server.GetValue() }

class WMSCapabilities(object):
    def __init__(self, cap_file):
        tree = etree.parse(cap_file)
        root_node = tree.getroot()
        
        self.xml_h = XMLHandlers(root_node)
        
        if not "version" in root_node.attrib:
            raise GException(_("Missing version attribute root node "
                               "in Capabilities XML file"))
        else:
            self.wms_version = root_node.attrib["version"]
        
        if self.wms_version == "1.3.0":
            self.proj_tag = "CRS"
        else:
            self.proj_tag = "SRS"

        self.cap_node = root_node.find(self.xml_h.ns("Capability"))
        if self.cap_node is None:
            raise GException(_("Missing <%s> tag in capabilities XML file"), "Capability")
        
        root_layer_node = self.cap_node.find(self.xml_h.ns("Layer"))
        if root_layer_node is None:
            raise GException(_("Missing <%s> tag in capabilities XML file"), "Layer")
        
        self.layers_by_id = {}
        self._initializeLayerTree(root_layer_node)
        
    def _initializeLayerTree(self, parent_layer, id = 0):
        if id == 0:
            parent_layer = WMSLayer(parent_layer, None, id, self.xml_h, self.proj_tag)
            self.layers_by_id[id] = parent_layer
            id += 1
        
        layer_nodes = parent_layer.layer_node.findall((self.xml_h.ns("Layer")))
        
        for layer_node in layer_nodes:
            layer = WMSLayer(layer_node, parent_layer, id, self.xml_h, self.proj_tag)
            self.layers_by_id[id] = layer
            id += 1
            id = self._initializeLayerTree(layer, id)
            parent_layer.child_layers.append(layer)
        
        return id


    def getFormats(self, event):
       
        request_node = self.cap_node.find(self.xml_h.ns("Request"))
        if  get_map_node is None:
            return None; 

        get_map_node = request_node.find(self.xml_h.ns("GetMap"))
        if  get_map_node is None:
            return None;
         
        format_nodes = get_map_node.findall(self.xml_h.ns("Format"))
 
        formats = []
        for node in format_nodes:
            formats.append[node.text]

        return formats

    def getProjIntersection(self, layers):

        intersec_projs = []


        firsIt = True
        for layer in layers:
            projs = layers[0].findall(self.xml_h.ns(self.proj_tag)) 

            if firsIt:  
                  intersec_projs = projs 
                  firsIt = False
                  continue

            tmp_intersec_proj
            for intersec_proj in intersec_projs:

                for proj in projs:

                    if proj.text == intersec_proj.text:

                        tmp_intersec_proj.append(proj)
                        break
        
            intersec_proj = tmp_intersec_proj

        projs = []
        for node in  intersec_proj:
            projs.append(intersec_proj.text)

        return projs

class WMSLayer(object):
    def __init__(self, layer_node, parent_layer, id, xml_h, proj_tag):
        self.id = id
        self.xml_h = xml_h
        self.proj_tag = proj_tag
        self.layer_node = layer_node
        self.parent_layer = parent_layer
        self.child_layers = []
        
        if parent_layer is not None:
            replaced_elements = [ ["EX_GeographicBoundingBox", "replace"],
                                  ["Attribution", "replace"],
                                  ["MinScaleDenominator", "replace"],
                                  ["MaxScaleDenominator", "replace"],
                                  ["AuthorityURL", "add"]]
            
            for element in replaced_elements:
                nodes = self.layer_node.findall(self.xml_h.ns(element[0]))

                if len(nodes) != 0 or element[1] == "add":
                    for node in parent_layer.layer_node.findall(self.xml_h.ns(element[0])):
                        self.layer_node.append(node)
            
            inh_arguments = ["queryable", "cascaded", "opaque",
                             "noSubsets", "fixedWidth", "fixedHeight"]
            
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
            
            is_there = False
            for node in nodes:
                cmp_text = None
                if cmp_type == "attribute":
                    if add_arg in node.attrib:
                        cmp_text = node.attrib[add_arg]
                
                elif cmp_type == "element_content":
                    cmp_text = node.text
                
                elif cmp_type == "child_element_content":
                    cmp_text = node.find(self.xml_h.ns(add_arg)).text
                
                if cmp_text.lower() == parent_cmp_text.lower():
                    is_there = True
                    break
            
            if not is_there:
                self.layer_node.append(parent_node)
