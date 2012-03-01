from osgeo import gdal
from wms_base import WMS
import grass.script as grass
import xml.etree.ElementTree as etree


class WMSGDAL(WMS):

    def __init__(self, options, flags):
        WMS.__init__(self, options, flags)
        self.bbox = self._computeRegion()
        self.temp_map = self._download()
	self._createOutputMap()

    def __del__(self):
        pass

    def _createXML(self):
        """!Create XML for GDAL WMS driver

        @return path to XML file
        """
        gdal_wms = etree.Element("GDAL_WMS")
        service = etree.SubElement(gdal_wms, "Service")
        name = etree.Element("name")
        service.set("name","WMS")
        version = etree.SubElement(service, "Version")
        version.text =self.o_wms_version
        server_url = etree.SubElement(service, "ServerUrl")
        server_url.text =self.o_mapserver_url

        if self.o_wms_version == "1.1.1":
            srs = etree.SubElement(service, "SRS")
        elif self.o_wms_version == "1.3.0":
            srs = etree.SubElement(service, "CRS")
        else: 
            grass.error("Unsupported wms version")
        srs.text = 'EPSG:' + self.o_srs

        image_format = etree.SubElement(service, "ImageFormat")
        image_format.text = "image/jpeg"#TODO!!

        layers = etree.SubElement(service, "Layers")
        layers.text =self.o_layers

        styles = etree.SubElement(service, "Styles")
        styles.text =self.o_styles

        data_window = etree.SubElement(gdal_wms, "DataWindow")
    
        self.xml_file = grass.tempfile()

        upper_left_x = etree.SubElement(data_window, "UpperLeftX")
        upper_left_x.text = self.bbox['w'] 

        upper_left_y = etree.SubElement(data_window, "UpperLeftY")
        upper_left_y.text = self.bbox['n'] 

        lower_right_x = etree.SubElement(data_window, "LowerRightX")
        lower_right_x.text = self.bbox['e'] 

        lower_right_y = etree.SubElement(data_window, "LowerRightY")
        lower_right_y.text = self.bbox['s']

        size_x = etree.SubElement(data_window, "SizeX")
        size_x.text = self.o_map_res_x 

        size_y = etree.SubElement(data_window, "SizeY")
        size_y.text = self.o_map_res_y 

        block_size_x = etree.SubElement(gdal_wms, "BlockSizeX")
        block_size_x.text = "400" #TODO!!

        block_size_y = etree.SubElement(gdal_wms, "BlockSizeY")
        block_size_y.text = "300"

        xml_file = grass.tempfile()
        if xml_file is None:
            grass.fatal("Unable to create temporary files")
        etree.ElementTree(gdal_wms).write(xml_file)

        return xml_file
    
    def _download(self):
        """!Downloads data from WMS server using GDAL WMS driver,
        store fetched data into temp file (self.temp_map)
        """
        temp_map = grass.tempfile()
        if temp_map is None:
            grass.fatal("Unable to create temporary files")

        xml_file = self._createXML()
        wms_dataset = gdal.Open(xml_file, gdal.GA_ReadOnly)
        grass.try_remove(xml_file)
        if wms_dataset is None:
            grass.fatal(_("Unable to open GDAL WMS driver"))
        
        #TODO!!
        format = "GTiff"
        driver = gdal.GetDriverByName(format)
        if wms_dataset is None:
            grass.fatal("can not find %s driver" % format)  
        tile_driver = driver.CreateCopy(temp_map, wms_dataset, 0)
        if wms_dataset is None:
            grass.fatal("can not open %s driver" % format)   
        tile_driver  = None
        wms_dataset = None

        return temp_map
