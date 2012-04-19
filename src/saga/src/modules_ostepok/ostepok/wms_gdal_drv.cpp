
#include "wms_import.h"

#include <wx/textfile.h>

#include <wx/xml/xml.h>
#include <gdal_priv.h>



CSG_String	CWMS_WMS_Gdal_drv::_Download		( void )
{

	CSG_String xmlPath = _CreateGdalDrvXml();

	GDALDataset  * pGdalWmsDataset;//TODO gdal C???

	GDALAllRegister();

	modul->Message_Add(SG_T("pred"));
	pGdalWmsDataset = (GDALDataset *) GDALOpen( xmlPath.b_str(), GA_ReadOnly );
	if( pGdalWmsDataset == NULL )
	{
		//return false;TODO
	}

	CSG_String GdalFormat = "GTiff";//TODO Gtif linkovaci problem

	GDALDriver * pGeoTiffDriver;
	pGeoTiffDriver = GetGDALDriverManager()->GetDriverByName( GdalFormat.b_str());
	pGdalWmsDataset = (GDALDataset *) GDALOpen( xmlPath.b_str(), GA_ReadOnly );
	if( pGdalWmsDataset == NULL )
	{
		modul->Message_Add(SG_T("nejede"));
		//return false;TODO
	}

	modul->Message_Add(SG_T("jede"));

	CSG_String tempMapPath = "pokus1.tiff";//TODO osetrit, smazat....

	GDALDataset  * pGeoTiffWmsDataset = pGeoTiffDriver->CreateCopy( tempMapPath.b_str()  , pGdalWmsDataset, FALSE,  NULL, NULL, NULL );

	pGeoTiffWmsDataset->FlushCache();
	modul->Message_Add(CSG_String(pGeoTiffWmsDataset->GetGCPProjection()));


	GDALClose( (GDALDataset *) pGdalWmsDataset );
	GDALClose( (GDALDataset *) pGeoTiffWmsDataset);


	return tempMapPath;


}

CSG_String     CWMS_WMS_Gdal_drv::_CreateGdalDrvXml		( void )
{

	wxXmlDocument gdalXML;
	wxXmlNode * ndGdalWMS;
	ndGdalWMS = new wxXmlNode(NULL,   wxXML_ELEMENT_NODE, SG_T("GDAL_WMS"));

	gdalXML.SetRoot (ndGdalWMS);

	wxXmlNode * ndService = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, SG_T("Service") );//TODO proc to padalo bez new

	ndService->AddAttribute(SG_T("name"), SG_T("WMS"));


	wxXmlNode * ndVersion = new wxXmlNode(ndService,   wxXML_ELEMENT_NODE, SG_T("Version"),SG_T("Service"));
	wxXmlNode * ndVersionContent = new wxXmlNode(ndVersion, wxXML_TEXT_NODE, SG_T("VersionContent"),  m_Capabilities.m_Version.c_str());

	modul->Message_Add(m_ServerUrl.c_str());
	wxXmlNode * ndServerURL =new wxXmlNode(ndService,   wxXML_ELEMENT_NODE, SG_T("ServerUrl"));
	wxXmlNode * ndServerURLContent = new wxXmlNode(ndServerURL, wxXML_TEXT_NODE, SG_T("ServerUrlContent"),  m_ServerUrl.c_str());

	modul->Message_Add(m_Format.c_str());

	wxXmlNode * ndImageFormat  = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, SG_T("ImageFormat"));
	wxXmlNode * ndImageFormatContent = new wxXmlNode(ndImageFormat, wxXML_TEXT_NODE, SG_T("ImageFormatContent"), m_Format.c_str());

	wxXmlNode * ndTransparent  = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, SG_T("Transparent"));
	wxXmlNode * ndTransparentContent = new wxXmlNode(ndTransparent, wxXML_TEXT_NODE, SG_T("TransparentContent"),  SG_T("True"));//TODO

//	wxXmlNode * ndTransparent; TODO
//	Service->AddChild (ndTransparent);
//	ndTransparent->SetName (SG_T("Transparent"));
//	ndTransparent->SetContent (m_Transparent);

	wxXmlNode * ndLayers = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, SG_T("Layers"));
	wxXmlNode * ndLayersContent = new wxXmlNode(ndLayers, wxXML_TEXT_NODE, SG_T("LayersContent"), m_Layers.c_str());

	wxXmlNode * ndStyles = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, SG_T("Styles"));;
	wxXmlNode * ndStylesContent = new wxXmlNode(ndStyles, wxXML_TEXT_NODE, SG_T("StylesContent"), m_Styles.c_str());

	wxXmlNode * ndSRS = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, m_Capabilities.m_ProjTag.c_str());
	wxXmlNode * ndSRSContent = new wxXmlNode(ndSRS, wxXML_TEXT_NODE, SG_T("SRSContent"), m_Proj.c_str());


	wxXmlNode * ndDataWindow = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, SG_T("DataWindow"));

	wxXmlNode * ndUpperLeftX = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, SG_T("UpperLeftX") );
	wxXmlNode * ndUpperLeftXContent = new wxXmlNode(ndUpperLeftX, wxXML_TEXT_NODE, SG_T("UpperLeftXContent"), CSG_String::Format(SG_T("%f"),wmsReqBBox.xMin).c_str() );

	wxXmlNode * ndUpperLeftY = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, SG_T("UpperLeftY") );
	wxXmlNode * ndUpperLeftYContent = new wxXmlNode(ndUpperLeftY, wxXML_TEXT_NODE, SG_T("UpperLeftYContent"), CSG_String::Format(SG_T("%f"), wmsReqBBox.yMax).c_str() );

	wxXmlNode * ndLowerRightX =new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, SG_T("LowerRightX") );
	wxXmlNode * ndLowerRightXContent = new wxXmlNode(ndLowerRightX, wxXML_TEXT_NODE, SG_T("LowerRightXContent"), CSG_String::Format(SG_T("%f"), wmsReqBBox.xMax).c_str() );


	wxXmlNode * ndLowerRightY = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, SG_T("LowerRightY") );
	wxXmlNode * ndLowerRightYContent = new wxXmlNode(ndLowerRightY, wxXML_TEXT_NODE, SG_T("LowerRightYContent"), CSG_String::Format(SG_T("%f"), wmsReqBBox.yMin).c_str() );

	wxXmlNode * ndSizeX = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, SG_T("SizeX") );
	wxXmlNode * ndSizeXContent = new wxXmlNode(ndSizeX, wxXML_TEXT_NODE, SG_T("SizeXContent"), CSG_String::Format(SG_T("%d"), m_sizeX).c_str());

	wxXmlNode * ndSizeY = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, SG_T("SizeY") );
	wxXmlNode * ndSizeYContent = new wxXmlNode(ndSizeY, wxXML_TEXT_NODE, SG_T("SizeXContent"), CSG_String::Format(SG_T("%d"), m_sizeY).c_str());

	wxXmlNode * ndBandsCount = new wxXmlNode(ndGdalWMS,   wxXML_ELEMENT_NODE, SG_T("BandsCount") );
	wxXmlNode * ndBandsCountContent = new wxXmlNode(ndBandsCount, wxXML_TEXT_NODE, SG_T("ndBandsCountContent"), CSG_String::Format(SG_T("%d"), 4).c_str());//TODO Block size

	wxXmlNode * ndBlockSizeX = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, SG_T("BlockSizeX") );
	wxXmlNode * ndBlockSizeXContent = new wxXmlNode(ndBlockSizeX, wxXML_TEXT_NODE, SG_T("BlockSizeXContent"), CSG_String::Format(SG_T("%d"), m_blockSizeX).c_str());

	wxXmlNode * ndBlockSizeY = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, SG_T("BlockSizeY") );
	wxXmlNode * ndBlockSizeYContent = new wxXmlNode(ndBlockSizeY, wxXML_TEXT_NODE, SG_T("BlockSizeYContent"), CSG_String::Format(SG_T("%d"), m_blockSizeY).c_str());

	CSG_String gdalDrvXmlPath("/home/ostepok/Desktop/xml.xml");
	gdalXML.Save(gdalDrvXmlPath.c_str());

	gdalXML.DetachRoot ();
	delete ndGdalWMS;

	wxTextFile xmlFile;
	xmlFile.Open(gdalDrvXmlPath.c_str());
	xmlFile.RemoveLine ((size_t) 0);
	xmlFile.Write();
//GetLineCount TODO
	return gdalDrvXmlPath;



}
