"""!
@package ogc_services.base

@brief Base classes OGC services

List of classes:
 - base::OGCDialog
 - base::LayersList
 - base::XMLHandlers

(C) 2009-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Stepan Turek <stepan.turek seznam.cz>
"""

import wx
from   wx.gizmos    import TreeListCtrl
import wx.lib.mixins.listctrl as listmix

class OGCDialog(wx.Dialog):
    def __init__(self, parent, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        """!Dialog to import data using OGC services (currently only
        WMS is implemented)"""
        self.parent = parent  # GMFrame 
        
        wx.Dialog.__init__(self, parent, id, style = style, **kwargs)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self._createWidgets()
        
        self._layout()
    
    def _createWidgets(self):
        pass
    
    def _doLayout(self):
        pass
    
class LayersList(TreeListCtrl, listmix.ListCtrlAutoWidthMixin):
    def __init__(self, parent, pos=wx.DefaultPosition):
        """!List of layers to be imported (dxf, shp...)"""
        self.parent = parent
        
        TreeListCtrl.__init__(self, parent, wx.ID_ANY,
                              style = wx.TR_DEFAULT_STYLE | wx.TR_HAS_BUTTONS |
                              wx.TR_FULL_ROW_HIGHLIGHT | wx.TR_MULTIPLE)
        
        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
        self.AddColumn(_('Layer / Style'))
        self.AddColumn(_('Title'))
        self.SetMainColumn(0) # column with the tree
        self.SetColumnWidth(0, 175)
        
        self.root = None
        
    def LoadData(self, cap = None):
        """!Load data into list"""
        # detete first all items
        self.DeleteAllItems()

        if not cap:
            return
          
        layers = cap.layers_by_id
        for id, layer in  layers.iteritems():

            prev_layer = layer

            title = layer.xml_h.ns("Title")
            name = layer.xml_h.ns("Name")
            if layer.layer_node.find(title) is not None:
                layer_name = layer.layer_node.find(title).text;
            elif layer.layer_node.find(name) is not None:
                layer_name = layer.layer_node.find(name).text
            else:
                layer_name = str(id);

            if id == 0:
                root_item = self.AddRoot(layer_name)
                layer_item = root_item    
                parent_item = root_item
                continue

            else:
                layer_item = self.AppendItem(parent_item, layer_name)
                self.SetPyData(layer_item, layer)

            style = layer.xml_h.ns("Style")
            for style_node in layer.layer_node.findall(style): 
                if style_node.find(title) is not None:
                    style_name = style_node.find(title).text
                elif style_node.find(name) is not None:
                    style_name = style_node.find(name).text
                else:
                    style_name = ""
                self.AppendItem(layer_item, style_name) 

            if id == 0:
                continue

            if id == len(layers) - 1:
                continue
            
            elif layers[id+1].parent_layer.id == 0:
                parent_item = root_item
                continue

            prev_is_parent = (prev_layer.id == layers[id + 1].parent_layer.id)
            prev_same_parent = (prev_layer.parent_layer.id == layers[id + 1].parent_layer.id)

            if not prev_is_parent and not prev_same_parent:
                parent_item = root_item
           
            if prev_is_parent:
                parent_item = layer_item 

            

    def GetItemCount(self):
        """!Required for listmix.ListCtrlAutoWidthMixin"""
        return 0

    def GetCountPerPage(self):
        """!Required for listmix.ListCtrlAutoWidthMixin"""
        return 0

    def GetSelectedLayers(self):
        """!Get selected layers/styles"""
        layers = dict()

        for item in self.GetSelections():
            parent = self.GetItemParent(item)
            if parent == self.root: # -> layer
                layer = self.GetItemText(item, 0)
                layers[layer] = list()
                sitem, cookie = self.GetFirstChild(item)
                while sitem:
                    layers[layer].append(self.GetItemText(sitem, 0))
                    sitem, cookie = self.GetNextChild(item, cookie)
            else: # -> style
                layer = self.GetItemText(parent, 0)
                layers[layer] = list()
                layers[layer].append(self.GetItemText(item, 0))
        
        return layers

class XMLHandlers(object):
    def __init__(self, root_node):
        self.namespace = "{http://www.opengis.net/wms}"
        
        if root_node.find("Service") is not None:
            self.use_ns = False
        
        if root_node.find(self.namespace + "Service") is not None:
            self.use_ns = True
        
        else:
            raise GException(_("Missing <%s> tag in capability XML file"), "Service")
    
    def ns(self, tag_name):
        if self.use_ns:
            tag_name = self.namespace + tag_name
        
        return tag_name
