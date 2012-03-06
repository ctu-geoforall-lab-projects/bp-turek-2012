import subprocess
from urllib2 import urlopen
import grass.script as grass
#try:
#    from grass.libs import gis as gislib
#except ImportError, e:
#    gislib = None
#    gislib_error = e

class WMS:
    
    def __init__(self, options, flags):
        
        self.o_mapserver_url = options['mapserver_url'] + "?"
        self.o_layers = options['layers']
        self.o_styles = options['styles']
        self.o_srs = options['srs']
        self.o_map_res_x = options['map_res_x'] ## TODO convert to int???
        self.o_map_res_y = options['map_res_y']
        self.o_output = options['output'] 
        self.o_region = options['region']
 	self.f_get_cap = flags["c"]
        
        self.o_wms_version = options['wms_version']        
        if self.o_wms_version == "1.1.1":
            self.projection_name = "SRS"
        elif self.o_wms_version == "1.3.0":
            self.projection_name = "CRS"
        else: 
            grass.error("Unsupported wms version")

        self.tile_x_size = 400
        self.tile_y_size = 300
        self.bbox = None # region extent for WMS query
        
	if self.o_region:                 
            if not grass.find_file(name = self.o_region, element = 'windows', mapset = '.' )['name']:
                grass.fatal(_("Region <%s> not found") % self.o_region)

        # read projection info
        self.proj_srs =      grass.read_command('g.proj', 
                                                flags = 'jf', 
                                                epsg = self.o_srs).rstrip('\n')
        self.proj_location = grass.read_command('g.proj', 
                                                flags='jf').rstrip('\n')
        
        if not self.proj_srs or not self.proj_location:
            grass.fatal(_("Unable to get projection info"))

    def __del__(self):
        # obnovit puvodni region pokud je to treba
        pass
        
    def _getCapabilities(self):
        """!Get capabilities from WMS server
        
        @return ElementTree instance of XML cap file
        """
        # download capabilities file
        cap_url_param = "service=WMS&request=GetCapabilities&version=" + self.o_wms_version
        cap_url = self.o_mapserver_url + cap_url_param 
        try:
            cap_file = urlopen(cap_url)
        except IOError:
            grass.fatal(_("Unable to get capabilities of '%s'") % self.o_mapserver_url)

        # check DOCTYPE first      
        if 'text/xml' not in cap_file.info()['content-type']:
            grass.fatal(_("Unable to get capabilities: %s. File is not XML.") % cap_url)

        # get element tree from XML file
        cap_xml = etree.parse(cap_file)
        cap_xml_service = cap_xml.find('Service')
	cap_xml_service_name = cap_xml_service.find('Name')
	if cap_xml_service_name.text != 'WMS':
            grass.fatal(_("Unable to get capabilities: %s. WMS not detected.") %  cap_url)
        
	return cap_xml

    def _computeBbox(self):
        """!Get region extent for WMS query (bbox)
        """
        if self.o_region:
            grass.run_command('g.region',
                              quiet = True,
                              region = self.o_region) ##TODO nastavit zpatky puvodni
        
 	region = grass.region()
        
        bbox = {'n' : None, 's' : None, 'e' : None, 'w' : None}
        
        if self.proj_srs == self.proj_location: # TODO: do it better
            for param in bbox:
                bbox[param] = region[param]
        ## zde se, pokud se lisi projekce wms a lokace, vypocita novy
        ## region, ktery vznikne transformaci regionu do zobrazeni wms
        ## a z techto ctyr transformovanych bodu se vyberou dva ktere
        ## maji extremni souradnice
        else:
            tmp_file_region_path = grass.tempfile()
            if tmp_file_region_path is None:
                grass.fatal(_("Unable to create temporary files"))
            tmp_file_region = open(tmp_file_region_path, 'w')
            tmp_file_region.write("%f %f\n%f %f\n%f %f\n%f %f\n"  %\
                                   (region['e'], region['n'],\
                                    region['w'], region['n'],\
                                    region['w'], region['s'],\
                                    region['e'], region['s'] ))
            tmp_file_region.close()
            
            points = grass.read_command('m.proj', flags = 'd',
                                        proj_output = self.proj_srs,
                                        proj_input = self.proj_location,
                                        input = tmp_file_region_path) # TODO: stdin
            grass.try_remove(tmp_file_region_path)
            if not points:
                grass.fatal(_("Unable to determine region, m.proj failed"))
                
            points = points.splitlines()
            if len(points) != 4:
                grass.fatal(_("Region defintion: 4 points required"))
            
            for point in points:
                point = map(float, point.split("|"))
                if not bbox['n']:
                    bbox['n'] = point[1]
                    bbox['s'] = point[1]
                    bbox['e'] = point[0]
                    bbox['w'] = point[0]
                    continue
                
                if   bbox['n'] < point[1]:
                    bbox['n'] = point[1]
                elif bbox['s'] > point[1]:
                    bbox['s'] = point[1]

                if   bbox['e'] < point[0]:
                    bbox['e'] = point[0]
                elif bbox['w'] > point[0]:
                    bbox['w'] = point[0]  
     
        return bbox

    def _createOutputMap(self): 
        """!Import downloaded data into GRASS, reproject data if needed
        using gdalwarp
        """
        if self.proj_srs != self.proj_location: # TODO: do it better
            temp_warpmap = grass.tempfile()

            ps = subprocess.Popen(['gdalwarp',
                                   '-s_srs', '%s' % self.proj_srs,
                                   '-t_srs', '%s' % self.proj_location,
                                   '-r', 'near',
                                   self.temp_map, temp_warpmap])
            ps.wait()
            if ps.returncode != 0:
                grass.fatal(_('gdalwarp failed'))
        else:
            temp_warpmap = self.temp_map

        if grass.run_command('r.in.gdal',
                             input = temp_warpmap,
                             output = self.o_output,
                             flags = 'k') != 0:
            grass.fatal(_('r.in.gdal failed'))
        grass.try_remove(self.temp_map)
        grass.try_remove(temp_warpmap)
        
        if grass.run_command('r.composite',
                             red =self.o_output + '.1',
                             green = self.o_output +  '.2',
                             blue = self.o_output + '.3',
                             output = self.o_output ) != 0:
                grass.fatal(_('r.composite failed'))
                

