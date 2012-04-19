#include "wms_import.h"

#include <wx/image.h>
#include <projects.h>

#include <gdalwarper.h>
#include "ogr_spatialref.h"
#define PROJ4_FREE(p)	if( p )	{	pj_free((PJ *)p);	p	= NULL;	}//TODO

bool		CWMS_WMS_Base::Get_Map		( void )
{
	_ComputeBbox	();
	CSG_String tempMapPath = _Download();
	_CreateOutputMap		( tempMapPath );

}




bool		CWMS_WMS_Base::_ComputeBbox	(void)
{
	if(m_bReProj)
	{
		projPJ src ;
		wxString proj(m_Proj.c_str());//TODO taky malym???
		wxString init (SG_T("+init="));
		proj = proj.MakeLower();


		modul->Message_Add(init+proj);

		if(!(src = pj_init_plus(SG_STR_SGTOMB(m_ReProj))))
		{
			return false;//TODO
		}
		modul->Message_Add( CSG_String(pj_get_def(src, 0)) );
		modul->Message_Add(m_ReProj);

		projPJ dst ;
		 if(!(dst = pj_init_plus(SG_STR_SGTOMB((init+proj)))))
		{
			return false;
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

			if   (wmsReqBBox.yMax < y[i_coorY]) wmsReqBBox.yMax = y[i_coorY];
			else if   (wmsReqBBox.yMin > y[i_coorY]) wmsReqBBox.yMin = y[i_coorY];

			if   (wmsReqBBox.xMax < x[i_coorX]) wmsReqBBox.xMax = x[i_coorX];
			else if   (wmsReqBBox.xMin > x[i_coorX]) wmsReqBBox.xMin = x[i_coorX];

		    }

		}
	}
	else wmsReqBBox = m_BBox;

	return true;//todo return bbox???

}





bool		CWMS_WMS_Base::_CreateOutputMap		( CSG_String & tempMapPath )
{
	_GdalWarp		( tempMapPath );
	wxImage	Image;
	if( Image.LoadFile(tempMapPath.c_str()) == false )
	{
		modul->Message_Add(_TL("could not read image"));

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



			    //-----------------------------------------
		pGrid->Set_Name(m_Capabilities.m_Title);//TODO layer name
		m_pOutputMap->Set_Value(pGrid);
		modul->DataObject_Set_Colors(pGrid, 100, SG_COLORS_BLACK_WHITE);

		CSG_Parameters Parameters;

		if( modul->DataObject_Get_Parameters(pGrid, Parameters) && Parameters("COLORS_TYPE") )
		{
			Parameters("COLORS_TYPE")->Set_Value(3);	// Color Classification Type: RGB
			modul->DataObject_Set_Parameters(pGrid, Parameters);
		}



    }


}

void	CWMS_WMS_Base::_GdalWarp		( CSG_String & tempMapPath )//TODO virtual
{


	GDALDataType eDT;//TODO test jestli je
	GDALDatasetH hDstDS;


	GDALDatasetH eoTiffWmsDataset;

	eoTiffWmsDataset = GDALOpen( tempMapPath.b_str(), GA_ReadOnly );
	   CPLAssert( eoTiffWmsDataset != NULL );


	// Create output with same datatype as first input band.

	eDT = GDALGetRasterDataType(GDALGetRasterBand(eoTiffWmsDataset,1));

	// Get output driver (GeoTIFF format)

	GDALDriverH GdalF = GDALGetDriverByName( "GTiff" );
	CPLAssert( GdalF != NULL );

	// Get Source coordinate system.

	const char *pszSrcWKT = NULL;
	char *pszDstWKT = NULL;

	pszSrcWKT = GDALGetProjectionRef( eoTiffWmsDataset );
	CPLAssert( pszSrcWKT != NULL && strlen(pszSrcWKT) > 0 );

	// Setup output coordinate system that is UTM 11 WGS84.

	OGRSpatialReference oSRS;

	oSRS.SetUTM( 30, TRUE );
	oSRS.SetWellKnownGeogCS( "WGS84" );

	oSRS.exportToWkt( &pszDstWKT );

	// Create a transformer that maps from source pixel/line coordinates
	// to destination georeferenced coordinates (not destination
	// pixel line).  We do that by omitting the destination dataset
	// handle (setting it to NULL).

	void *hTransformArg;

	hTransformArg =
	    GDALCreateGenImgProjTransformer( eoTiffWmsDataset, pszSrcWKT, NULL, pszDstWKT,
					     FALSE, 0, 1 );
	CPLAssert( hTransformArg != NULL );

	// Get approximate output georeferenced bounds and resolution for file.

	double adfDstGeoTransform[6];
	int nPixels=0, nLines=0;
	CPLErr eErr;

	eErr = GDALSuggestedWarpOutput( eoTiffWmsDataset,
					GDALGenImgProjTransform, hTransformArg,
					adfDstGeoTransform, &nPixels, &nLines );
	CPLAssert( eErr == CE_None );

	GDALDestroyGenImgProjTransformer( hTransformArg );

	// Create the output file.

	hDstDS = GDALCreate( GdalF, "pokus1.tiff", nPixels, nLines,
			     GDALGetRasterCount(eoTiffWmsDataset), eDT, NULL );//TODO cesta

	CPLAssert( hDstDS != NULL );

	// Write out the projection definition.

	GDALSetProjection( hDstDS, pszDstWKT );
	GDALSetGeoTransform( hDstDS, adfDstGeoTransform );

	// Copy the color table, if required.

	GDALColorTableH hCT;

	hCT = GDALGetRasterColorTable( GDALGetRasterBand(eoTiffWmsDataset,1) );
	if( hCT != NULL )
	    GDALSetRasterColorTable( GDALGetRasterBand(hDstDS,1), hCT );



	GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();

	  psWarpOptions->hSrcDS  = eoTiffWmsDataset;
	  psWarpOptions->hDstDS = hDstDS;

	  psWarpOptions->nBandCount = 0;


	  psWarpOptions->pfnProgress = GDALTermProgress;

	  // Establish reprojection transformer.

	  psWarpOptions->pTransformerArg =
	      GDALCreateGenImgProjTransformer( eoTiffWmsDataset,
					       GDALGetProjectionRef(eoTiffWmsDataset),
					       hDstDS,
					       GDALGetProjectionRef(hDstDS),
					       FALSE, 0.0, 1 );
	  psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

	  // Initialize and execute the warp operation.

	  GDALWarpOperation oOperation;

	  oOperation.Initialize( psWarpOptions );
	  oOperation.ChunkAndWarpImage( 0, 0,
					GDALGetRasterXSize( hDstDS ),
					GDALGetRasterYSize( hDstDS ) );

	  GDALDestroyGenImgProjTransformer( psWarpOptions->pTransformerArg );
	  GDALDestroyWarpOptions( psWarpOptions );

	  GDALClose( hDstDS );
	  GDALClose( eoTiffWmsDataset );



}
