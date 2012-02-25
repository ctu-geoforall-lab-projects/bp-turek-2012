import grass.script as grass
import xml.etree.ElementTree as etree
from urllib2 import urlopen
from osgeo import gdal
from osgeo import gdalconst
from osgeo import osr
import subprocess

class WMS:

    def run(self, options, flags):
       
        self.__initializeParameters(options, flags)
        self.__computeRegion()
        self.__downloadTiles()
	self.__createOutputMapFromTiles()


    def __initializeParameters(self, options, flags):

        self.o_mapserver_url = options['mapserver_url'] + "?"
        self.o_wms_version = options['wms_version']
        self.o_layers = options['layers']
        self.o_styles = options['styles']
        self.o_srs = options['srs']
        self.o_map_res_x = options['map_res_x']
        self.o_map_res_y = options['map_res_y']
        self.o_output = options['output'] 
 	self.f_get_cap = flags["c"]
        self.tiles = []
        # check if given region exists      
	if options['region']: ##TODO, 
            if not grass.find_file(name = options['region'], element = 'windows')['name']:
                grass.fatal(_("Region <%s> not found") % options['region'])

        ##proc tam musi byt rstrip
        self.proj_srs = grass.read_command('g.proj', 
                                           flags='jf', 
                                           epsg =self.o_srs ).rstrip('\n')
        self.proj_location = grass.read_command('g.proj', 
                                                flags='jf').rstrip('\n')

    def __getCapabilities(self):

    # download capabilities file
        cap_url_param = "service=WMS&request=GetCapabilities&version=" + self.o_wms_version
        cap_url = self.o_mapserver_url + cap_url_param 
        try:
                cap_file = urlopen(cap_url )
        except IOError:
            grass.fatal(_("Unable to get capabilities of '%s'") % self.o_mapserver_url)

   # check DOCTYPE first      
        if 'text/xml' not in cap_file.info()['content-type']:
            grass.fatal(_("Unable to get capabilities: %s") % cap_url)
        cap_xml = etree.parse(cap_file)
        cap_xml_service = cap_xml.find('Service')
	cap_xml_service_name = cap_xml_service.find('Name')
	if cap_xml_service_name.text != 'WMS':
            grass.fatal(_("Unable to get capabilities: %s") %  cap_url)		
	return cap_xml


    def __computeRegion(self):

	region = grass.region()
        self.bbox = dict({'n' : '', 's' : '', 'e' : '', 'w' : ''});

        if self.proj_srs ==  self.proj_location:
            for param in self.bbox:
                self.bbox[param] = str(region[param])
        ## zde se, pokud se lisi projekce wms a lokace, vypocita novy region, ktery vznikne transformaci regionu do zobrazeni wms a z techto ctyr transformovanych bodu se vyberou dva ktere maji extremni souradnice
        else:           
            tmp_file_region_path = grass.tempfile()
            if  tmp_file_region_path is None:
                 grass.fatal("Unable to create temporary files")
            tmp_file_region = open( tmp_file_region_path, 'w')
            tmp_file_region.write("%f %f\n%f %f\n%f %f\n%f %f\n"  %\
                                   (region['e'], region['n'],\
                                    region['w'], region['n'],\
                                    region['w'], region['s'],\
                                    region['e'], region['s'] ) )
            tmp_file_region.close()
            points = grass.read_command('m.proj', flags = 'd',
                                    proj_output = self.proj_srs,
                                    proj_input = self.proj_location,
                                    input =  tmp_file_region_path)
            points = points.splitlines()
            for point in points:
                point = point.split("|")
                if not self.bbox['n']:
                    self.bbox['n'] = point[1]
                    self.bbox['s'] = point[1]
                    self.bbox['e'] = point[0]
                    self.bbox['w'] = point[0]
                    continue

                if   float(self.bbox['n']) < float(point[1]):
                    self.bbox['n'] = point[1]
                elif float(self.bbox['s']) > float(point[1]):
                    self.bbox['s'] = point[1]

                if   float(self.bbox['e']) < float(point[0]):
                    self.bbox['e'] = point[0]
                elif float(self.bbox['w']) > float(point[0]):
                    self.bbox['w'] = point[0]  

    def __downloadTiles(self):
        ## vysledkem bude 1 tile, protoze o tilovani se nam uz postara gdal wms driver
        ## u modulu bez wms driveru muze byt vysledkem vice tilu,  proto nazev   __downloadTiles 
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

        tile = grass.tempfile()
        if xml_file is None:
            grass.fatal("Unable to create temporary files")

        wms_dataset = gdal.Open( xml_file, gdal.GA_ReadOnly )
        if wms_dataset is None:
            grass.fatal("can not open wms driver")  
      
        #TODO!!
        format = "GTiff"
        driver = gdal.GetDriverByName( format )
        if wms_dataset is None:
            grass.fatal("can not find %s driver" % format)  
        tile_driver = driver.CreateCopy( tile, wms_dataset, 0 )
        if wms_dataset is None:
            grass.fatal("can not open %s driver" % format)   
        tile_driver  = None
        wms_dataset = None

        self.tiles.append(tile)

     
    def __createOutputMapFromTiles(self): 
       
        for tile in self.tiles:
            if self.proj_srs !=  self.proj_location:
                warptile = grass.tempfile()
                # neni python binding pro gdal warp,  aspon to pisou tady: http://www.gdal.org/warptut.html
                ps = subprocess.Popen(['gdalwarp',
                                       '-s_srs', '%s' % self.proj_srs,
                                       '-t_srs', '%s' % self.proj_location,
                                       '-r', 'near',
                                       tile, warptile])
                ps.wait()
                if ps.returncode != 0:
                    grass.fatal(_('gdalwarp failed'))
            else:
                warptile = tile

            if grass.run_command('r.in.gdal',
                                 input = warptile,
                                 output = self.o_output + '_tile_' + '1',
                                 flags = 'k') != 0:
                grass.fatal(_('r.in.gdal failed'))

    
        #TODO r.patch  - bude potreba az pro driver bez gdal wms
        i = 1  
        if grass.run_command('r.composite',
                             red =self.o_output + '_tile_' + str(i) + '.1',
                             green =self.o_output + '_tile_' + str(i) + '.2',
                             blue =self.o_output + '_tile_' + str(i) + '.3',
                             output = self.o_output ) != 0:
                grass.fatal(_('r.composite failed'))


