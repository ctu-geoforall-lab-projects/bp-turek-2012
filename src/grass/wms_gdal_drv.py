from osgeo import gdal
import sys
from osgeo import gdalconst
from wms_base import WMSBASE
import grass.script as grass
import xml.etree.ElementTree as etree

##gdal.UseExceptions() 

class WMSGDALDRV(WMSBASE):

    def __del__(self):
        pass

    def _createXML(self):
        """!Create XML for GDAL WMS driver

        @return path to XML file
        """
        self._debug("_createXML", "started")
        
        gdal_wms = etree.Element("GDAL_WMS")
        service = etree.SubElement(gdal_wms, "Service")
        name = etree.Element("name")
        service.set("name","WMS")
        version = etree.SubElement(service, "Version")
        version.text =self.o_wms_version
        server_url = etree.SubElement(service, "ServerUrl")
        server_url.text =self.o_wms_server_url
      
        srs = etree.SubElement(service, self.projection_name)   
        srs.text = 'EPSG:' + str(self.o_srs)

        image_format = etree.SubElement(service, "ImageFormat")
        image_format.text = self.mime_format

        image_format = etree.SubElement(service, "Transparent")
        image_format.text = self.transparent

        layers = etree.SubElement(service, "Layers")
        layers.text =self.o_layers

        styles = etree.SubElement(service, "Styles")
        styles.text =self.o_styles

        data_window = etree.SubElement(gdal_wms, "DataWindow")
    
        self.xml_file = grass.tempfile()

        upper_left_x = etree.SubElement(data_window, "UpperLeftX")
        upper_left_x.text = str(self.bbox['w']) 

        upper_left_y = etree.SubElement(data_window, "UpperLeftY")
        upper_left_y.text = str(self.bbox['n']) 

        lower_right_x = etree.SubElement(data_window, "LowerRightX")
        lower_right_x.text = str(self.bbox['e']) 

        lower_right_y = etree.SubElement(data_window, "LowerRightY")
        lower_right_y.text = str(self.bbox['s'])

        size_x = etree.SubElement(data_window, "SizeX")
        size_x.text = str(self.region['cols']) 

        size_y = etree.SubElement(data_window, "SizeY")
        size_y.text = str(self.region['rows']) 

        block_size_x = etree.SubElement(gdal_wms, "BlockSizeX")
        block_size_x.text = str(self.o_maxcols) 

        block_size_y = etree.SubElement(gdal_wms, "BlockSizeY")
        block_size_y.text = str(self.o_maxrows)

        xml_file = grass.tempfile()
        if xml_file is None:
            grass.fatal(_("Unable to create temporary files"))
        etree.ElementTree(gdal_wms).write(xml_file)

        self._debug("_createXML", "finished -> %s" % xml_file)
        
        return xml_file
    
    def _download(self):
        """!Downloads data from WMS server using GDAL WMS driver

        @return temp_map with stored downloaded data
        """
        self._debug("_download", "started")
        temp_map = grass.tempfile()
        if temp_map is None:
            grass.fatal(_("Unable to create temporary files"))

        xml_file = self._createXML()
        wms_dataset = gdal.Open(xml_file, gdal.GA_ReadOnly)
        grass.try_remove(xml_file)
        if wms_dataset is None:
            grass.fatal(_("Unable to open GDAL WMS driver"))

        self._debug("_download", "GDAL dataset created")
        
        driver = gdal.GetDriverByName(self.gdal_drv_format)
        if driver is None:
            grass.fatal(_("Unable to find %s driver" % format))

        metadata = driver.GetMetadata()
        if not metadata.has_key(gdal.DCAP_CREATECOPY) or \
           metadata[gdal.DCAP_CREATECOPY] == 'NO':
            grass.fatal(_('Driver %s supports CreateCopy() method.') % self.gdal_drv_name)

        self._debug("_download", "calling GDAL CreateCopy...")
        ##try:
        temp_map_dataset = driver.CreateCopy(temp_map, wms_dataset, 0)
        ##except RuntimeError as err:
          ## grass.fatal(err)
        if temp_map_dataset is None:
            grass.fatal(_("Incorrect WMS query"))

        temp_map_dataset  = None
        wms_dataset = None

        self._debug("_download", "finished")

        return temp_map
