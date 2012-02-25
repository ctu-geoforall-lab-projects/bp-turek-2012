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
#% description: WMS 
#% keywords: WMS
#% keywords: web map service
#%end

#%option
#% key: wms_version
#% type:string
#% description:WMS standard
#% options:1.1.1,1.3.0
#% answer:1.1.1
#% required: yes
#%end


#%option
#% key: mapserver_url
#% type: string
#% description:URL of WMS server 
#% required: yes
#% answer: http://wms.cuzk.cz/wms.asp
#% guisection: Request
#%end

#%option
#% key: layers
#% type: string
#% description: Layers to request from map server
#% multiple: yes
#% required: yes
#% answer: prehledka_kraju-linie
#% guisection: Request
#%end

#%option
#% key: styles
#% type: string
#% description: Styles to request from map server
#% multiple: yes
#% required: no
#% guisection: Request
#%end

#%option
#% key: srs
#% type: string
#% description: Source projection to request from server
#% answer:32633
#% guisection: Request
#%end

#%option
#% key: region
#% type: string
#% description: Named region to request data for. Current region used if omitted
#% required : no
#% guisection: Request
#%end

#%option
#% key: map_res_x
#% type:integer
#% description: X resolution of map
#% answer:800
#% guisection: Request
#%end

#%option
#% key: map_res_y
#% type:integer
#% description: Y resolution of map
#% answer:600
#% guisection: Request
#%end

#%option
#% key: format
#% type: string
#% description: Image format requested from the server
#% options: geotiff,tiff,jpeg,gif,png
#% answer: geotiff
#% required: yes
#% guisection: Request
#%end

#%option G_OPT_R_OUTPUT
#% required: yes
#% description: output map
#%end

#%flag
#% key: o
#% description: Don't request transparent data
#% guisection: Request
#%end

#%flag
#% key: c
#% description: Get capabilities
#% guisection: Request
#%end


import sys
import atexit

import grass.script as grass

from wms_base import WMS

def cleanup():
    maps = []
    for suffix in ('1', '2', '3'):
        rast = options['output'] + '_tile_' + '1.' + suffix
        if grass.find_file(rast, element = 'cell', mapset = '.')['file']:
            maps.append(rast)
    
    if maps:
        grass.run_command('g.remove',
                          quiet = True,
                          rast = ','.join(maps))
    
def main():
    pokus = WMS()
    pokus.run(options, flags)
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
