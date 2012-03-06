from osgeo import gdal
from osgeo import gdalconst
from wms_base import WMS
import grass.script as grass
from urllib2 import urlopen


class WMSDRV(WMS):

    # TODO prace s chybama
    # TODO stahovat tily mimop bbox?
    ## nebylo by lepsi umistit do wms_base?
    def __init__(self, options, flags):

        WMS.__init__(self, options, flags)
        self.bbox = self._computeBbox()
        self.temp_map = self._download()
	self._createOutputMap()

    def _download(self):
        """!Downloads data from WMS server using own driver

        @return temp_map with stored downloaded data
        """
        
        ## computes number of tiles in x and y axises 
        num_tiles_x = int(self.o_map_res_x) / int(self.tile_x_size)  ## TODO porad pretypovani?, tile_x_size > o_map_res_x
        last_tile_x_size = int(self.o_map_res_x) % int(self.tile_x_size) 
        tile_x_length =  float(self.tile_x_size) / float(self.o_map_res_x ) * (self.bbox['e'] - self.bbox['w']) 

        last_tile_x = False
        if last_tile_x_size != 0:
            last_tile_x = True
            num_tiles_x = num_tiles_x + 1

        num_tiles_y = int(self.o_map_res_y) / int(self.tile_y_size) 
        last_tile_y_size = int(float(self.o_map_res_y)) % int(self.tile_y_size)
        tile_y_length =  float(self.tile_y_size) / float(self.o_map_res_y) * (self.bbox['n'] - self.bbox['s']) 

        last_tile_y = False
        if last_tile_y_size != 0:
            last_tile_y = True
            num_tiles_y = num_tiles_y + 1

        tile_bbox = dict(self.bbox)
        tile_bbox['e'] = self.bbox['w']  + tile_x_length
        

        ## downloads each tile and writes it into temp_map 
        tile_to_temp_map_size_x = self.tile_x_size 
        i_x = 0   
        while i_x < num_tiles_x:
            if i_x != 0:
                tile_bbox['e'] = tile_bbox['e'] + tile_x_length 
                tile_bbox['w'] = tile_bbox['w'] + tile_x_length            

            if i_x == num_tiles_x - 1 and last_tile_x:
                tile_to_temp_map_size_x = last_tile_x_size 

            tile_bbox['n'] = self.bbox['n']                    
            tile_bbox['s'] = self.bbox['n']  - tile_y_length 
    
            tile_to_temp_map_size_y = self.tile_y_size       
            i_y = 0
            while i_y < num_tiles_y:        
                if i_y != 0:
                    tile_bbox['s'] = tile_bbox['s'] - tile_y_length 
                    tile_bbox['n'] = tile_bbox['n'] - tile_y_length

                if i_y == num_tiles_y - 1 and last_tile_y:
                    tile_to_temp_map_size_y = last_tile_y_size 

                url = self.o_mapserver_url + "REQUEST=GetMap&VERSION=%s&LAYERS=%s&WIDTH=%s&HEIGHT=%s" %\
                      (self.o_wms_version, self.o_layers, self.tile_x_size, self.tile_y_size )
                query_bbox = "BBOX=%s,%s,%s,%s" % (tile_bbox['w'], tile_bbox['s'], tile_bbox['e'], tile_bbox['n'])
                proj = self.projection_name + "=EPSG:"+ self.o_srs

                url = url + "&" + query_bbox + "&" +proj+ "&" + "FORMAT=image/jpeg"
                wms_data= urlopen( url)

                temp_tile = grass.tempfile()
                if temp_tile is None:
                    grass.fatal(_("Unable to create temporary files"))

                temp_tile_opened = open(temp_tile, 'w')
                temp_tile_opened.write(wms_data.read())
                temp_tile_opened.close()                    
                tile_dataset = gdal.Open(temp_tile, gdal.GA_ReadOnly)
                
                if i_x == 0 and i_y == 0:
                    temp_map = grass.tempfile()
                    if temp_map is None:
                        grass.fatal(_("Unable to create temporary files"))

                    format = "GTiff" ## TODO
                    driver = gdal.GetDriverByName(format)
                    band = tile_dataset.GetRasterBand(1)
                    cell_type_func = band.__swig_getmethods__["DataType"]# nejde to vyresit elegantneji?  
                    bands_number_func = tile_dataset.__swig_getmethods__["RasterCount"]
                    temp_map_dataset = driver.Create(temp_map, int(self.o_map_res_x), int(self.o_map_res_y), \
                                                     bands_number_func(tile_dataset), cell_type_func(band) );
	
                tile_to_temp_map = tile_dataset.ReadRaster(0, 0, tile_to_temp_map_size_x, tile_to_temp_map_size_y, \
                                                                 tile_to_temp_map_size_x, tile_to_temp_map_size_y) 	
                temp_map_dataset.WriteRaster(self.tile_x_size * i_x, self.tile_y_size * i_y, \
                                             tile_to_temp_map_size_x,  tile_to_temp_map_size_y, tile_to_temp_map)   
                tile_dataset = None
                grass.try_remove(temp_tile)                       
                i_y = i_y + 1
            i_x = i_x + 1

        projection = grass.read_command('g.proj', 
                                        flags = 'wf',
                                        epsg =self.o_srs).rstrip('\n')
        temp_map_dataset.SetProjection(projection)

        pixel_x_length = (self.bbox['e'] - self.bbox['w']) /  int(self.o_map_res_x)
        pixel_y_length = (self.bbox['s'] - self.bbox['n']) /  int(self.o_map_res_y)
        geo_transform = [ self.bbox['w'] , pixel_x_length  , 0.0 ,  self.bbox['n'] , 0.0 , pixel_y_length ] 
        temp_map_dataset.SetGeoTransform(geo_transform )
        temp_map_dataset = None
        return temp_map

