#include "wms_import.h"

#include <projects.h>

#include <wx/image.h>
#include <wx/stdpaths.h>

#include <gdalwarper.h>
#include "ogr_spatialref.h"

#define PROJ4_FREE(p)	if( p )	{	pj_free((PJ *)p);	p	= NULL;	}//TODO


CWMS_WMS_Base::CWMS_WMS_Base(void)
{
	wxStandardPaths StdPaths;
	m_TempDir = StdPaths.GetTempDir();
}


CWMS_WMS_Base::~CWMS_WMS_Base(void){}


void		CWMS_WMS_Base::Get_Map		( void )
{
// public function. which calls private functons
	wmsReqBBox =		_ComputeBbox();

	wxString tempMapPath =	_Download();//abstract function, possible to define various wms drivers

				_CreateOutputMap(tempMapPath);
}


TSG_Rect        CWMS_WMS_Base::_ComputeBbox	(void)
{
	TSG_Rect wmsReqBBox;
	if(m_bReProj && m_bReProjBbox)
	{
	// destination bbox points are transformed into wms
	// projection and then bbox is created from extreme coordinates
	// of the transformed points
		projPJ src ;
		wxString proj(m_Proj.c_str());//TODO taky malym???
		wxString init (SG_T("+init="));
		proj = proj.MakeLower();

		if(!(src = pj_init_plus(SG_STR_SGTOMB(m_ReProj))))
		{
			   throw WMS_Exception(wxT("Wrong PROJ4 format of destination projection."));
		}

		projPJ dst ;
		if(!(dst = pj_init_plus(SG_STR_SGTOMB((init+proj)))))
		{
		    throw WMS_Exception(wxT("Wrong PROJ4 format:") ); //+ m_ReProj());
		}

		const int size = 2;
		double x[size] = {m_BBox.xMax, m_BBox.xMin};
		double y[size] = {m_BBox.yMax, m_BBox.yMin};


		pj_transform( src, dst, size, 0, x, y, NULL );
		PROJ4_FREE(src);
		PROJ4_FREE(dst);

		for(int i_coorX = 0; i_coorX < size; i_coorX++)
		{

			for(int i_coorY = 0; i_coorY < size; i_coorY++)
			{
			    if(i_coorY == 0 && i_coorX == 0)
			    {
				wmsReqBBox.yMax = y[i_coorY];
				wmsReqBBox.yMin = y[i_coorY];
				wmsReqBBox.xMax = x[i_coorX];
				wmsReqBBox.xMin = x[i_coorX];
				continue;
			    }

			    if   (wmsReqBBox.yMax > y[i_coorY]) wmsReqBBox.yMax = y[i_coorY];
			    else if   (wmsReqBBox.yMin < y[i_coorY]) wmsReqBBox.yMin = y[i_coorY];

			    if   (wmsReqBBox.xMax > x[i_coorX]) wmsReqBBox.xMax = x[i_coorX];
			    else if   (wmsReqBBox.xMin < x[i_coorX]) wmsReqBBox.xMin = x[i_coorX];
			}
		}
	}

	// no transformation required
	else wmsReqBBox = m_BBox;

	return wmsReqBBox;
}





void	CWMS_WMS_Base::_CreateOutputMap(wxString & tempMapPath)
{
	if(m_bReProj)    _GdalWarp		(tempMapPath);

	wxImage	Image;
	if( Image.LoadFile(tempMapPath) == false )
	{
		throw WMS_Exception(wxT("Unable read image from:") + tempMapPath);

	}
	else
	{
		CSG_Grid	*pGrid	= SG_Create_Grid(SG_DATATYPE_Int, Image.GetWidth(), Image.GetHeight(), 1, m_BBox.xMin, m_BBox.yMin);

		for(int y=0, yy=pGrid->Get_NY()-1; y<pGrid->Get_NY(); y++, yy--)
		{
			for(int x=0; x<pGrid->Get_NX(); x++)
			{
				pGrid->Set_Value(x, y, SG_GET_RGB(Image.GetRed(x, yy), Image.GetGreen(x, yy), Image.GetBlue(x, yy)));

				if (!Image.GetAlpha(x, yy))
				{
				    pGrid->Set_NoData(x, y );
				}
			}
		}

		pGrid->Set_Name(m_Capabilities.m_Title);//TODO layer name
		m_pOutputMap->Set_Value(pGrid);
		modul->DataObject_Set_Colors(pGrid, 100, SG_COLORS_BLACK_WHITE);

		CSG_Parameters Parameters;

		if( modul->DataObject_Get_Parameters(pGrid, Parameters) && Parameters("COLORS_TYPE") )//TODO
		{
			Parameters("COLORS_TYPE")->Set_Value(3);	// Color Classification Type: RGB
			modul->DataObject_Set_Parameters(pGrid, Parameters);
		}
    }


}

void	CWMS_WMS_Base::_GdalWarp		( wxString & tempMapPath )//TODO virtual
{
// reprojectiong raster into demanded projection, modified code from http://www.gdal.org/warptut.html

	GDALDataType tempMapDataType;
	GDALDatasetH warpedTemMapDataset;


	GDALDatasetH tempMapDataset;

	tempMapDataset = GDALOpen(tempMapPath.mb_str(), GA_ReadOnly );


	// Create output with same datatype as first input band.

	tempMapDataType = GDALGetRasterDataType(GDALGetRasterBand(tempMapDataset,1));

	// Get output driver (GeoTIFF format)
	wxString  gdalDrvFormat = wxT("GTiff");//TODO
	GDALDriverH GdalF = GDALGetDriverByName(gdalDrvFormat.mb_str());
	if(!GdalF)
	{
		throw WMS_Exception(wxT("Unable to find driver:") +  gdalDrvFormat);
	}

	// Get Source coordinate system.

	const char *pSrcWKT = NULL;
	char *pDstWKT = NULL;

	pSrcWKT = GDALGetProjectionRef(tempMapDataset);
	if(!pSrcWKT && strlen(pSrcWKT))
	{
		throw WMS_Exception(wxT("Unable to get projection from downloaded file."));
	}

	// Setup output coordinate system that is UTM 11 WGS84.

	OGRSpatialReference oSRS;

	oSRS.importFromProj4 (m_ReProj.mb_str());

	oSRS.exportToWkt( &pDstWKT );

	// Create a transformer that maps from source pixel/line coordinates
	// to destination georeferenced coordinates (not destination
	// pixel line).  We do that by omitting the destination dataset
	// handle (setting it to NULL).

	void *hTransformArg;

	hTransformArg =
	    GDALCreateGenImgProjTransformer( tempMapDataset, pSrcWKT, NULL, pDstWKT,
					     FALSE, 0, 1 );
	if(!hTransformArg)
	{
		throw WMS_Exception(wxT("Unable to create GDALCreateGenImgProjTransformer."));
	}
	// Get approximate output georeferenced bounds and resolution for file.

	double adfDstGeoTransform[6];
	int nPixels=0, nLines=0;
	CPLErr eErr;

	eErr = GDALSuggestedWarpOutput( tempMapDataset,
					GDALGenImgProjTransform, hTransformArg,
					adfDstGeoTransform, &nPixels, &nLines );
	//CPLAssert( eErr == CE_None ); TODO

	GDALDestroyGenImgProjTransformer( hTransformArg );

	// Create the output file.

	warpedTemMapDataset = GDALCreate( GdalF, tempMapPath.mb_str(), nPixels, nLines,
			     GDALGetRasterCount(tempMapDataset), tempMapDataType, NULL );//TODO cesta

	// Write out the projection definition.

	GDALSetProjection( warpedTemMapDataset, pDstWKT );
	GDALSetGeoTransform( warpedTemMapDataset, adfDstGeoTransform );

	// Copy the color table, if required.

	GDALColorTableH hCT;

	hCT = GDALGetRasterColorTable( GDALGetRasterBand(tempMapDataset,1) );
	if( hCT != NULL )
	    GDALSetRasterColorTable( GDALGetRasterBand(warpedTemMapDataset,1), hCT );

	GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();

	psWarpOptions->hSrcDS  = tempMapDataset;
	psWarpOptions->hDstDS = warpedTemMapDataset;

	psWarpOptions->nBandCount = 0;
	psWarpOptions->eResampleAlg = (GDALResampleAlg)m_ReProjMethod;

	psWarpOptions->pfnProgress = GDALTermProgress;

	  // Establish reprojection transformer.

	psWarpOptions->pTransformerArg =
	GDALCreateGenImgProjTransformer( tempMapDataset,
			       GDALGetProjectionRef(tempMapDataset),
			       warpedTemMapDataset,
			       GDALGetProjectionRef(warpedTemMapDataset),
			       FALSE, 0.0, 1 );
	psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

	// Initialize and execute the warp operation.

	GDALWarpOperation oOperation;

	oOperation.Initialize( psWarpOptions );
	oOperation.ChunkAndWarpImage( 0, 0,
			GDALGetRasterXSize( warpedTemMapDataset ),
			GDALGetRasterYSize( warpedTemMapDataset ) );

	GDALDestroyGenImgProjTransformer( psWarpOptions->pTransformerArg );
	GDALDestroyWarpOptions( psWarpOptions );

	GDALClose( warpedTemMapDataset );
	GDALClose( tempMapDataset );
}
