import subprocess
from urllib2 import urlopen
import grass.script as grass


class WMS:


    def _initializeParameters(self, options, flags):

        self.o_mapserver_url = options['mapserver_url'] + "?"
        self.o_wms_version = options['wms_version']
        self.o_layers = options['layers']
        self.o_styles = options['styles']
        self.o_srs = options['srs']
        self.o_map_res_x = options['map_res_x']
        self.o_map_res_y = options['map_res_y']
        self.o_output = options['output'] 
        self.o_region = options['region']
 	self.f_get_cap = flags["c"]
        # check if given region exists 
        self.region = []     
	if self.o_region:                 
            if not grass.find_file(name = self.o_region, element = 'windows', mapset = '.' )['name']:
                grass.fatal(_("Region <%s> not found") % self.o_region)

        ##proc tam musi byt rstrip
        self.proj_srs =      grass.read_command('g.proj', 
                                                flags='jf', 
                                                epsg =self.o_srs ).rstrip('\n')
        self.proj_location = grass.read_command('g.proj', 
                                                flags='jf').rstrip('\n')


    def _getCapabilities(self):

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


    def _computeRegion(self):

        if self.region:
            grass.run_command('g.region',
                              quiet = True,
                              region = options['region']) ## nastavit zpatky puvodni?
        else:
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
            grass.try_remove(tmp_file_region_path)
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

     
    def _createOutputMap(self): 
       
        if self.proj_srs !=  self.proj_location:
            temp_warpmap = grass.tempfile()
            # neni python binding pro gdal warp,  aspon to pisou tady: http://www.gdal.org/warptut.html
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
                             green =self.o_output +  '.2',
                             blue =self.o_output + '.3',
                             output = self.o_output ) != 0:
                grass.fatal(_('r.composite failed'))


