#include "wms_import.h"

#include <projects.h>
#include <gdalwarper.h>
#include "ogr_spatialref.h"

#include <wx/filename.h>

#define PROJ4_FREE(p)	if( p )	{	pj_free((PJ *)p);	p	= NULL;	}


CWMS_Base::CWMS_Base(void){}

CWMS_Base::~CWMS_Base(void){}


wxString	CWMS_Base::Get_Map(void)
{
// public function. which calls private functons
	wmsReqBBox =		_ComputeBbox();

	wxString tempMapPath;
	tempMapPath = _Download();//abstract function, possible definition various WMS drivers
	if(m_bReProj)  tempMapPath =  _GdalWarp		(tempMapPath);

	return tempMapPath;
}


TSG_Rect        CWMS_Base::_ComputeBbox(void)
{
	TSG_Rect wmsReqBBox;
	wxString proj(m_Proj.c_str());
	proj = proj.MakeLower();

	wxString init (SG_T("+init="));

	projPJ dst = pj_init_plus(SG_STR_SGTOMB((init+proj)));

	if(m_bReProj && m_bReProjBbox)
	{
	// destination bbox points are transformed into wms
	// projection and then bbox is created from extreme coordinates
	// of the transformed points
		if(!dst)
		{
		    throw CWMS_Exception(wxT("Wrong projection format.") ); //+ m_ReProj());
		}

		projPJ src ;
		if(!(src = pj_init_plus(SG_STR_SGTOMB(m_ReProj))))
		{
			throw CWMS_Exception(wxT("Wrong PROJ4 format of destination projection."));
		}

		m_LayerProjDef =  wxString::FromUTF8(pj_get_def(src, 0));
		const int size = 2;
		double x[size] = {m_BBox.xMax, m_BBox.xMin};
		double y[size] = {m_BBox.yMax, m_BBox.yMin};

		pj_transform( src, dst, size, 0, x, y, NULL );
		PROJ4_FREE(src);
		PROJ4_FREE(dst);

		for(int i_CoorX = 0; i_CoorX < size; i_CoorX++)
		{
			for(int i_CoorY = 0; i_CoorY < size; i_CoorY++)
			{
			    if(i_CoorY == 0 && i_CoorX == 0)
			    {
				wmsReqBBox.yMax = y[i_CoorY];
				wmsReqBBox.yMin = y[i_CoorY];
				wmsReqBBox.xMax = x[i_CoorX];
				wmsReqBBox.xMin = x[i_CoorX];
				continue;
			    }

			    if   (wmsReqBBox.yMax < y[i_CoorY]) wmsReqBBox.yMax = y[i_CoorY];
			    else if   (wmsReqBBox.yMin > y[i_CoorY]) wmsReqBBox.yMin = y[i_CoorY];

			    if   (wmsReqBBox.xMax < x[i_CoorX]) wmsReqBBox.xMax = x[i_CoorX];
			    else if   (wmsReqBBox.xMin > x[i_CoorX]) wmsReqBBox.xMin = x[i_CoorX];
			}
		}
	}

	// no transformation required
	else
	{	wmsReqBBox = m_BBox;
		if(dst) m_LayerProjDef = wxString::FromUTF8(pj_get_def(dst, 0));
	}
	return wmsReqBBox;
}


wxString	CWMS_Base::_GdalWarp(const wxString & tempMapPath)
{
// reprojectiong raster into demanded projection, modified code from http://www.gdal.org/warptut.html

	GDALDataType tempMapDataType;
	GDALDatasetH warpedTemMapDataset;

	GDALDatasetH tempMapDataset;

	tempMapDataset = GDALOpen(tempMapPath.mb_str(), GA_ReadOnly );
	if(tempMapDataset == NULL)
	{
		throw CWMS_Exception(wxT("Unable to create dataset."));
	}

	// Create output with same datatype as first input band.

	tempMapDataType = GDALGetRasterDataType(GDALGetRasterBand(tempMapDataset,1));

	// Get output driver (GeoTIFF format)
	wxString  gdalDrvFormat = wxT("GTiff");
	GDALDriverH GdalF = GDALGetDriverByName(gdalDrvFormat.mb_str());
	if(!GdalF)
	{
		throw CWMS_Exception(wxT("Unable to find driver:") +  gdalDrvFormat);
	}

	// Get Source coordinate system.

	const char *pSrcWKT = NULL;
	char *pDstWKT = NULL;

	pSrcWKT = GDALGetProjectionRef(tempMapDataset);
	if(!pSrcWKT && strlen(pSrcWKT))
	{
		throw CWMS_Exception(wxT("Unable to get projection from downloaded file."));
	}

	// Setup output coordinate system

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
		throw CWMS_Exception(wxT("Unable to create GDALCreateGenImgProjTransformer."));
	}
	// Get approximate output georeferenced bounds and resolution for file.

	double adfDstGeoTransform[6];
	int nPixels=0, nLines=0;
	CPLErr eErr;

	eErr = GDALSuggestedWarpOutput( tempMapDataset,
					GDALGenImgProjTransform, hTransformArg,
					adfDstGeoTransform, &nPixels, &nLines );
	//TODO CPLAssert eErr

	GDALDestroyGenImgProjTransformer( hTransformArg );

	// Create the output file.
	wxString warpedTempMapPath =  wxFileName::CreateTempFileName(wxT("SAGAWmsWarpedTempMap"));
	warpedTemMapDataset = GDALCreate( GdalF,  warpedTempMapPath.mb_str(), nPixels, nLines,
			     GDALGetRasterCount(tempMapDataset), tempMapDataType, NULL );

	if(warpedTemMapDataset == NULL)
	{
		throw CWMS_Exception(wxT("Unable to create dataset."));
	}
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

	//TODO CE_Failure

	GDALDestroyGenImgProjTransformer( psWarpOptions->pTransformerArg );
	GDALDestroyWarpOptions( psWarpOptions );

	GDALClose( warpedTemMapDataset );
	GDALClose( tempMapDataset );

	wxRemoveFile(tempMapPath);

	return warpedTempMapPath;
}


