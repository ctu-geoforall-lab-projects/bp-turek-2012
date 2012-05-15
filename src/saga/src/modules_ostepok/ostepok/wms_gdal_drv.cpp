#include "wms_import.h"

#include <wx/textfile.h>
#include <wx/filename.h>

#include <gdal_priv.h>


wxString	CWMS_Gdal_drv::_Download(void)
{
	wxString xmlPath = _CreateGdalDrvXml();

	GDALDataset  * pGdalWmsDataset;
	GDALAllRegister();

	pGdalWmsDataset = (GDALDataset *) GDALOpen( xmlPath.mb_str(), GA_ReadOnly );
	if(!pGdalWmsDataset)
	{
		throw CWMS_Exception(wxT("Unable to create dataset"));
	}

	wxString gdalDrvFormat = wxT("GTiff");

	GDALDriver * pGeoTiffDriver;
	pGeoTiffDriver = GetGDALDriverManager()->GetDriverByName(gdalDrvFormat.mb_str());
	if(!pGeoTiffDriver)
	{
		throw CWMS_Exception(wxT("Unable to find driver:") + gdalDrvFormat);
	}

	wxString tempMapPath = wxFileName::CreateTempFileName(wxT("SAGAWmsTempMap"));


	char ** driverMetadata = pGeoTiffDriver->GetMetadata();

	if( !CSLFetchBoolean(driverMetadata, GDAL_DCAP_CREATECOPY, FALSE))
	{
		throw CWMS_Exception(wxT("Driver") + gdalDrvFormat + wxT("supports CreateCopy() method.") );
	}

	GDALDataset  * pGeoTiffWmsDataset = pGeoTiffDriver->CreateCopy( tempMapPath.mb_str(), pGdalWmsDataset, FALSE,  NULL, NULL, NULL );

	if(!pGdalWmsDataset)
	{
		throw CWMS_Exception(wxT("Unable to create create wms driver"));
	}


	GDALClose( (GDALDataset *) pGdalWmsDataset );
	GDALClose( (GDALDataset *) pGeoTiffWmsDataset);
	wxRemoveFile(xmlPath);

	return tempMapPath;


}

wxString     CWMS_Gdal_drv::_CreateGdalDrvXml		( void )
{

	wxXmlDocument gdalXML;
	wxXmlNode * ndGdalWMS;
	ndGdalWMS = new wxXmlNode(NULL,   wxXML_ELEMENT_NODE, wxT("GDAL_WMS"));

	gdalXML.SetRoot (ndGdalWMS);

	wxXmlNode * ndService = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, wxT("Service") );

	ndService->AddAttribute(wxT("name"), wxT("WMS"));

	wxXmlNode * ndVersion = new wxXmlNode(ndService,   wxXML_ELEMENT_NODE, wxT("Version"),wxT("Service"));
	wxXmlNode * ndVersionContent = new wxXmlNode(ndVersion, wxXML_TEXT_NODE, wxT("VersionContent"),  m_Capabilities.m_Version.c_str());

	wxXmlNode * ndServerURL =new wxXmlNode(ndService,   wxXML_ELEMENT_NODE, wxT("ServerUrl"));
	wxXmlNode * ndServerURLContent = new wxXmlNode(ndServerURL, wxXML_TEXT_NODE, wxT("ServerUrlContent"),  m_ServerUrl.c_str());

	wxXmlNode * ndImageFormat  = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, wxT("ImageFormat"));
	wxXmlNode * ndImageFormatContent = new wxXmlNode(ndImageFormat, wxXML_TEXT_NODE, wxT("ImageFormatContent"), m_Format.c_str());

	wxXmlNode * ndTransparent  = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, wxT("Transparent"));
	wxXmlNode * ndTransparentContent = new wxXmlNode(ndTransparent, wxXML_TEXT_NODE, wxT("TransparentContent"),  m_bTransparent? wxT("true"): wxT("false"));


	wxXmlNode * ndLayers = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, wxT("Layers"));
	wxXmlNode * ndLayersContent = new wxXmlNode(ndLayers, wxXML_TEXT_NODE, wxT("LayersContent"), m_Layers.c_str());

	wxXmlNode * ndStyles = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, wxT("Styles"));;
	wxXmlNode * ndStylesContent = new wxXmlNode(ndStyles, wxXML_TEXT_NODE, wxT("StylesContent"), m_Styles.c_str());

	wxXmlNode * ndSRS = new wxXmlNode(ndService, wxXML_ELEMENT_NODE, m_Capabilities.m_ProjTag.c_str());
	wxXmlNode * ndSRSContent = new wxXmlNode(ndSRS, wxXML_TEXT_NODE, wxT("SRSContent"), m_Proj.c_str());

	wxXmlNode * ndDataWindow = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, wxT("DataWindow"));

	wxXmlNode * ndUpperLeftX = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, wxT("UpperLeftX") );
	wxXmlNode * ndUpperLeftXContent = new wxXmlNode(ndUpperLeftX, wxXML_TEXT_NODE, wxT("UpperLeftXContent"), wxString::Format(wxT("%f"),wmsReqBBox.xMin).c_str() );

	wxXmlNode * ndUpperLeftY = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, wxT("UpperLeftY") );
	wxXmlNode * ndUpperLeftYContent = new wxXmlNode(ndUpperLeftY, wxXML_TEXT_NODE, wxT("UpperLeftYContent"), wxString::Format(wxT("%f"), wmsReqBBox.yMax).c_str() );

	wxXmlNode * ndLowerRightX =new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, wxT("LowerRightX") );
	wxXmlNode * ndLowerRightXContent = new wxXmlNode(ndLowerRightX, wxXML_TEXT_NODE, wxT("LowerRightXContent"), wxString::Format(wxT("%f"), wmsReqBBox.xMax).c_str() );


	wxXmlNode * ndLowerRightY = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, wxT("LowerRightY") );
	wxXmlNode * ndLowerRightYContent = new wxXmlNode(ndLowerRightY, wxXML_TEXT_NODE, wxT("LowerRightYContent"), wxString::Format(wxT("%f"), wmsReqBBox.yMin).c_str() );

	wxXmlNode * ndSizeX = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, wxT("SizeX") );
	wxXmlNode * ndSizeXContent = new wxXmlNode(ndSizeX, wxXML_TEXT_NODE, wxT("SizeXContent"), wxString::Format(wxT("%d"), m_sizeX).c_str());

	wxXmlNode * ndSizeY = new wxXmlNode(ndDataWindow, wxXML_ELEMENT_NODE, wxT("SizeY") );
	wxXmlNode * ndSizeYContent = new wxXmlNode(ndSizeY, wxXML_TEXT_NODE, wxT("SizeXContent"), wxString::Format(wxT("%d"), m_sizeY).c_str());

	// 4 - RGB + A (transparency layer)
	wxXmlNode * ndBandsCount = new wxXmlNode(ndGdalWMS,   wxXML_ELEMENT_NODE, wxT("BandsCount") );
	wxXmlNode * ndBandsCountContent = new wxXmlNode(ndBandsCount, wxXML_TEXT_NODE, wxT("ndBandsCountContent"), wxString::Format(wxT("%d"), 4).c_str());

	wxXmlNode * ndBlockSizeX = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, wxT("BlockSizeX") );
	wxXmlNode * ndBlockSizeXContent = new wxXmlNode(ndBlockSizeX, wxXML_TEXT_NODE, wxT("BlockSizeXContent"), wxString::Format(wxT("%d"), m_blockSizeX).c_str());

	wxXmlNode * ndBlockSizeY = new wxXmlNode(ndGdalWMS, wxXML_ELEMENT_NODE, wxT("BlockSizeY") );
	wxXmlNode * ndBlockSizeYContent = new wxXmlNode(ndBlockSizeY, wxXML_TEXT_NODE, wxT("BlockSizeYContent"), wxString::Format(wxT("%d"), m_blockSizeY).c_str());

	wxString gdalXmlPath = wxFileName::CreateTempFileName(wxT("SAGAgdal"));
	wxTextFile xmlFile;
	xmlFile.Create(gdalXmlPath);

	gdalXML.Save(gdalXmlPath);

	gdalXML.DetachRoot ();
	delete ndGdalWMS;//Removes whole tree

	xmlFile.Open(gdalXmlPath);

	if(xmlFile.GetLineCount() > 1)	xmlFile.RemoveLine ((size_t) 0);
	else				throw CWMS_Exception(wxT("Corrupted XML file for GDAL Driver"));

	xmlFile.Write();//TODO wxInvalidOffset

	return gdalXmlPath;
}
