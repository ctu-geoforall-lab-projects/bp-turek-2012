#!/usr/bin/env python
"""
    MODULE:    r.wms
    
    AUTHOR(S): Stepan Turek <stepan.turek AT seznam.cz>
    
    PURPOSE:   Describe your script here...
    
    COPYRIGHT: (C) 2012 Stepan Turek, and by the GRASS Development Team
    
               This program is free software under the GNU General Public
               License (>=v2). Read the file COPYING that comes with GRASS
               for details.
    """

#%module
#% description: Downloads and imports data from WMS servers.
#% keywords: raster
#% keywords: import
#% keywords: wms
#%end

#%option
#% key: mapserver_url
#% type: string
#% description:URL of WMS server 
#% required: yes
#% answer: http://wms.cuzk.cz/wms.asp
#%end

#%option
#% key: layers
#% type: string
#% description: Layers to request from map server
#% multiple: yes
#% required: yes
#% answer: prehledka_kraju-linie
#%end

#%option G_OPT_R_OUTPUT
#% description: Name for output raster map
#%end

#%option
#% key: styles
#% type: string
#% description: Styles to request from map server
#% multiple: yes
#% guisection: Map style
#%end

#%option
#% key: bgcolor
#% type: string
#% description: Color of map background (only with 'd' flag)
#% guisection: Map style
#%end

#%option
#% key: srs
#% type: integer
#% description: EPSG number of source projection for request 
#% answer:32633
#% guisection: Request properties
#%end

#%option
#% key: region
#% type: string
#% description: Named region to request data for. Current region used if omitted
#% guisection: Request properties
#%end

#%option
#% key: wms_version
#% type:string
#% description:WMS standard
#% options:1.1.1,1.3.0
#% answer:1.1.1
#% guisection: Request properties
#%end

#%option
#% key: format
#% type: string
#% description: Image format requested from the server
#% options: geotiff,tiff,jpeg,gif,png
#% answer: geotiff
#% guisection: Request properties
#%end

#%option
#% key: maxcols
#% type:integer
#% description: Maximum columns to request at a time
#% answer:400
#% guisection: Request properties
#%end

#%option
#% key: maxrows
#% type: integer
#% description: Maximum rows to request at a time
#% answer: 300
#% guisection: Request properties
#%end

#%flag
#% key: t
#% description: Transparent background
#% guisection: Map style
#%end

#%flag
#% key: c
#% description: Get capabilities
#% guisection: Request properties
#% suppress_required: yes
#%end

#%flag
#% key: d
#% description: Do not use GDAL WMS driver
#%end

import sys
import atexit

import grass.script as grass

from wms_drv import WMSDRV
from wms_gdal_drv import WMSGDALDRV

def cleanup(): 
    maps = []
    for suffix in ('.red', '.green', '.blue', '.alpha'):
        rast = options['output'] + suffix
        if grass.find_file(rast, element = 'cell', mapset = '.')['file']:
            maps.append(rast)
    
    if maps:
        grass.run_command('g.remove',
                          quiet = True,
                          flags='f',
                          rast = ','.join(maps))
    
def main():
    if flags['d']:
        grass.debug("Using own driver")
        wms = WMSDRV(options, flags)
    else:
        grass.debug("Using GDAL WMS driver")
        wms = WMSGDALDRV(options, flags)
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
