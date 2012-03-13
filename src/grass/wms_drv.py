#TODO test importu gdal
from osgeo import gdal
from osgeo import gdalconst
from wms_base import WMSBASE
import grass.script as grass
from urllib2 import urlopen
import numpy as Numeric
Numeric.arrayrange = Numeric.arange


class WMSDRV(WMSBASE):

    def _download(self):
        """!Downloads data from WMS server using own driver

        @return temp_map with stored downloaded data
        """    
        cols = self.region['cols']
        rows = self.region['rows']
        
        # Compute parameters of tiles 
        num_tiles_x = cols / self.o_maxcols  
        last_tile_x_size = cols % self.o_maxcols 
        tile_x_length =  float(self.o_maxcols) / float(cols ) * (self.bbox['e'] - self.bbox['w']) 

        last_tile_x = False
        if last_tile_x_size != 0:
            last_tile_x = True
            num_tiles_x = num_tiles_x + 1

        num_tiles_y = rows / self.o_maxrows 
        last_tile_y_size = rows % self.o_maxrows
        tile_y_length =  float(self.o_maxrows) / float(rows) * (self.bbox['n'] - self.bbox['s']) 

        last_tile_y = False
        if last_tile_y_size != 0:
            last_tile_y = True
            num_tiles_y = num_tiles_y + 1
        
        ## Downloads each tile and writes it into temp_map 
        proj = self.projection_name + "=EPSG:"+ str(self.o_srs)
        url = self.o_wms_server_url + "REQUEST=GetMap&VERSION=%s&LAYERS=%s&WIDTH=%s&HEIGHT=%s&STYLES=%s&BGCOLOR=%s&TRASPARENT=%s" %\
                  (self.o_wms_version, self.o_layers, self.o_maxcols, self.o_maxrows, self.o_styles, self.o_bgcolor, self.transparent)
        url = url + "&" +proj+ "&" + "FORMAT=" + self.mime_format
        
        tile_bbox = dict(self.bbox)
        tile_bbox['e'] = self.bbox['w']  + tile_x_length

        tile_to_temp_map_size_x = self.o_maxcols 
        for i_x in range(num_tiles_x):
            # set bbox for tile i_x,i_y (E, W)
            if i_x != 0:
                tile_bbox['e'] += tile_x_length 
                tile_bbox['w'] += tile_x_length            
            
            if i_x == num_tiles_x - 1 and last_tile_x:
                tile_to_temp_map_size_x = last_tile_x_size 
            
            tile_bbox['n'] = self.bbox['n']                    
            tile_bbox['s'] = self.bbox['n'] - tile_y_length 
            tile_to_temp_map_size_y = self.o_maxrows       
            
            for i_y in range(num_tiles_y):
                # set bbox for tile i_x,i_y (N, S)
                if i_y != 0:
                    tile_bbox['s'] -= tile_y_length 
                    tile_bbox['n'] -= tile_y_length
                
                if i_y == num_tiles_y - 1 and last_tile_y:
                    tile_to_temp_map_size_y = last_tile_y_size 
                
                # bbox for tile defined
                query_url = url + "&" + "BBOX=%s,%s,%s,%s" % (tile_bbox['w'], tile_bbox['s'], tile_bbox['e'], tile_bbox['n'])
                wms_data = urlopen(query_url)
                
                temp_tile = grass.tempfile()
                if temp_tile is None:
                    grass.fatal(_("Unable to create temporary files"))
                
                # download data into temporary file
                temp_tile_opened = open(temp_tile, 'w')##TODO osetrit vyjimky
                temp_tile_opened.write(wms_data.read())
                temp_tile_opened.close()       
                
                tile_dataset = gdal.Open(temp_tile, gdal.GA_ReadOnly) 
                if tile_dataset is None:
                    error_file_opened = open(temp_tile, 'r')##TODO osetrit vyjimky
                    err_str = error_file_opened.read()
                     
                    if  err_str is not None:
                        grass.fatal(_("Server WMS error: %s") %  err_str)
                    else:
                        grass.fatal(_("Server WMS unknown error") )
                
                band = tile_dataset.GetRasterBand(1) 
                cell_type_func = band.__swig_getmethods__["DataType"]# zkousel jsem bez  __swig_getmethods__ a neslo to
                bands_number_func = tile_dataset.__swig_getmethods__["RasterCount"]
                
                ## Expansion of color table (if exists) into bands 

                ######stejny problem resili v r.in.wms - soubor gdalwarp.py radek 117####
                temp_tile_pct2rgb = None
                if bands_number_func(tile_dataset) == 1 and band.GetRasterColorTable() is not None:
                    temp_tile_pct2rgb = grass.tempfile()
                    if temp_tile_pct2rgb is None:
                        grass.fatal(_("Unable to create temporary files"))
                    tile_dataset = self._pct2rgb(temp_tile, temp_tile_pct2rgb)##TODO

                ## Initialization of temp_map_dataset, where all tiles are merged
                if i_x == 0 and i_y == 0:
                    temp_map = grass.tempfile()
                    if temp_map is None:
                        grass.fatal(_("Unable to create temporary files"))  
                  
                    driver = gdal.GetDriverByName(self.gdal_drv_format)
                    metadata = driver.GetMetadata()
                    if not metadata.has_key(gdal.DCAP_CREATE) or \
                           metadata[gdal.DCAP_CREATE] == 'NO':
                        grass.fatal(_('Driver %s supports Create() method.') % drv_format)  
                    
                    temp_map_dataset = driver.Create(temp_map, int(cols), int(rows),
                                                     bands_number_func(tile_dataset), cell_type_func(band));

                ## write tile into temp map
                tile_to_temp_map = tile_dataset.ReadRaster(0, 0, tile_to_temp_map_size_x, tile_to_temp_map_size_y,
                                                                 tile_to_temp_map_size_x, tile_to_temp_map_size_y)
 	
                temp_map_dataset.WriteRaster(self.o_maxcols * i_x, self.o_maxrows * i_y,
                                             tile_to_temp_map_size_x,  tile_to_temp_map_size_y, tile_to_temp_map) 
  
                tile_dataset = None
                grass.try_remove(temp_tile)
                grass.try_remove(temp_tile_pct2rgb)    
        
        # Georeferencing and setting projection of temp_map
        projection = grass.read_command('g.proj', 
                                        flags = 'wf',
                                        epsg =self.o_srs).rstrip('\n')
        temp_map_dataset.SetProjection(projection)

        pixel_x_length = (self.bbox['e'] - self.bbox['w']) / int(cols)
        pixel_y_length = (self.bbox['s'] - self.bbox['n']) / int(rows)
        geo_transform = [ self.bbox['w'] , pixel_x_length  , 0.0 ,  self.bbox['n'] , 0.0 , pixel_y_length ] 
        temp_map_dataset.SetGeoTransform(geo_transform )
        temp_map_dataset = None

        return temp_map


    def _pct2rgb(self, src_filename, dst_filename):
        """!Create new dataset with data in dst_filename with bands according to src_filename 
            raster color table 

        @return new dataset
        """  

        format = 'GTiff'
        out_bands = 3
        band_number = 1
        
        # Open source file
        src_ds = gdal.Open( src_filename )
        if src_ds is None:
            grass.fatal(_('Unable to open %s ' % src_filename))

        src_band = src_ds.GetRasterBand(band_number)

        # Build color table.
        lookup = [ Numeric.arrayrange(256), 
                   Numeric.arrayrange(256), 
                   Numeric.arrayrange(256), 
                   Numeric.ones(256)*255 ]

        ct = src_band.GetRasterColorTable()	
        if ct is not None:
            for i in range(min(256,ct.GetCount())):
                entry = ct.GetColorEntry(i)
                for c in range(4):
                    lookup[c][i] = entry[c]

        # Create the working file.
        gtiff_driver = gdal.GetDriverByName(format)
        tif_ds = gtiff_driver.Create( dst_filename,
                                      src_ds.RasterXSize, src_ds.RasterYSize, out_bands )

        # Do the processing one scanline at a time. 
        for iY in range(src_ds.RasterYSize):
            src_data = src_band.ReadAsArray(0,iY,src_ds.RasterXSize,1)

            for iBand in range(out_bands):
                band_lookup = lookup[iBand]
    
                dst_data = Numeric.take(band_lookup,src_data)
                tif_ds.GetRasterBand(iBand+1).WriteArray(dst_data,0,iY)

        return tif_ds       
