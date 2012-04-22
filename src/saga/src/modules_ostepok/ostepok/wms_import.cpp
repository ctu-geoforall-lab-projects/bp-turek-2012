/**********************************************************
 * Version $Id: table.cpp 911 2011-02-14 16:38:15Z reklov_w $
 *********************************************************/

///////////////////////////////////////////////////////////
//                                                       //
//                         SAGA                          //
//                                                       //
//      System for Automated Geoscientific Analyses      //
//                                                       //
//                    Module Library:                    //
//                   garden_webservices                  //
//                                                       //
//-------------------------------------------------------//
//                                                       //
//                     wms_import.cpp                    //
//                                                       //
//                 Copyright (C) 2011 by                 //
//                StepanTurek, Olaf Conrad               //
//                                                       //
//-------------------------------------------------------//
//                                                       //
// This file is part of 'SAGA - System for Automated     //
// Geoscientific Analyses'. SAGA is free software; you   //
// can redistribute it and/or modify it under the terms  //
// of the GNU General Public License as published by the //
// Free Software Foundation; version 2 of the License.   //
//                                                       //
// SAGA is distributed in the hope that it will be       //
// useful, but WITHOUT ANY WARRANTY; without even the    //
// implied warranty of MERCHANTABILITY or FITNESS FOR A  //
// PARTICULAR PURPOSE. See the GNU General Public        //
// License for more details.                             //
//                                                       //
// You should have received a copy of the GNU General    //
// Public License along with this program; if not,       //
// write to the Free Software Foundation, Inc.,          //
// 59 Temple Place - Suite 330, Boston, MA 02111-1307,   //
// USA.                                                  //
//                                                       //
//-------------------------------------------------------//
//                                                       //
//    e-mail:     oconrad@saga-gis.org                   //
//                                                       //
//    contact:    Olaf Conrad                            //
//                Institute of Geography                 //
//                University of Hamburg                  //
//                Germany                                //
//                                                       //
///////////////////////////////////////////////////////////

//---------------------------------------------------------


#include "wms_import.h"
#include <wx/xml/xml.h>
#include "cpl_port.h"

CWMS_Import::CWMS_Import(void)
{

	Set_Name		(_TL(" WMS"));

	Set_Author		(SG_T("S. Turek (c) 2012"));

	Set_Description	(_TW(
		"This module works as Web Map Service (WMS) client. "
		"More information on the WMS specifications can be obtained from the "
		"Open Geospatial Consortium (OGC) at "
		"<a href=\"http://www.opengeospatial.org/\">http://www.opengeospatial.org/</a>. "
	));

	//-----------------------------------------------------
	Parameters.Add_Grid_Output(
		NULL	, "MAP"				, _TL("WMS Map"),
		_TL("")
	);

	//-----------------------------------------------------
	Parameters.Add_String(
		NULL	, "SERVER"			, _TL("Server"),
		_TL(""),
	//	SG_T("www.gaia-mv.de/dienste/DTK10f")	// 260000.0x, 5950000.0y Cellsize 1.0
	//	SG_T("www.gis2.nrw.de/wmsconnector/wms/stobo")
	//	SG_T("www2.demis.nl/mapserver/request.asp")
	//	SG_T("www.geoserver.nrw.de/GeoOgcWms1.3/servlet/TK25")
	//	SG_T("www.geographynetwork.com/servlet/com.esri.wms.Esrimap")
		SG_T("ogc.bgs.ac.uk/cgi-bin/BGS_Bedrock_and_Superficial_Geology/wms")	// WGS84: Center -3.5x 55.0y Cellsize 0.005
	);

	Parameters.Add_String(
		NULL	, "USERNAME"		, _TL("User Name"),
		_TL(""),
		SG_T("")
	);

	Parameters.Add_String(
		NULL	, "PASSWORD"		, _TL("Password"),
		_TL(""),
		SG_T(""), false, true
	);

	m_WMS = NULL;

}

//---------------------------------------------------------
CWMS_Import::~CWMS_Import(void)
{

	delete m_WMS;

}


bool CWMS_Import::On_Execute(void)
{

	// new class
	try
	{
		m_WMS = new CWMS_WMS_Gdal_drv;
	}
	catch(std::bad_alloc& except)
	{
		Message_Add(CSG_String::Format(SG_T("ERROR: Unable to allocate memomry: %s"), except.what()));
		return false;
	}


	m_WMS->m_ServerUrl	= Parameters("SERVER")->asString();

	if(m_WMS->m_ServerUrl.Contains(wxT("http://")))
	{
		m_WMS->m_ServerUrl		= Parameters("SERVER")->asString() + 7;
	}


	wxString ServerUrl	= m_WMS->m_ServerUrl.BeforeFirst(SG_T('/'));

	m_WMS->m_Capabilities.m_Server.SetUser		(Parameters("USERNAME")->asString());
	m_WMS->m_Capabilities.m_Server.SetPassword	(Parameters("PASSWORD")->asString());

	if(m_WMS->m_Capabilities.m_Server.Connect(ServerUrl.c_str()) == false)
	{
		Message_Add(_TL("ERROR: Unable to connect to server."));
		return( false );
	}


	// GetCapabilities request to server
	try
	{
	    m_WMS->m_Capabilities.Create(m_WMS);
	}
	catch(WMS_Exception &except)
	{
	    Message_Add((CSG_String("ERROR:") + CSG_String(except.what())));
	    return false;
	}
	catch(std::bad_alloc& except)
	{
	    Message_Add(CSG_String::Format(SG_T("ERROR: Unable to allocate memomry: %s"), except.what()));
	    return false;
	}


	std::vector<CWMS_Layer*> layers;

	m_WMS->m_Capabilities.Get_Layers(layers);

	CSG_Parameters	parLayers;
	Create_Layers_Dialog( layers, parLayers);

	std::vector<CWMS_Layer*> selectedLayers;
	do
	{
		// show layers dialog
		if( Dlg_Parameters(&parLayers, _TL("Choose layers"))) { }

		for(int i=0; i<layers.size(); i++)
		{
		       if(parLayers(CSG_String::Format(SG_T("layer_%d"), layers[i]->m_id) )->asBool())
			{
				selectedLayers.push_back(layers[i]);
			}
		}

		if(selectedLayers.size() < 1) Message_Dlg(_TL("No layers choosen"));
	}
	while(selectedLayers.size() < 1);

	CSG_Parameters	parSettings;
	Create_Settings_Dialog(selectedLayers, parSettings);

	// show dialog with other request settings
	if( Dlg_Parameters(&parSettings, _TL("WMS Request settings"))) { }


	// assing chosen values to m_WMS class members
	wxString style;
	for(int i_layer = 0; i_layer < selectedLayers.size(); i_layer++)
	{
		CSG_Parameter * pStyleChoicePar = parSettings.Get_Parameter( wxString::Format(SG_T("style_%d"),  selectedLayers[i_layer]->m_id));
		if(pStyleChoicePar)
		{
			style = pStyleChoicePar->asString();
		}
		else style = wxT("");
		if(i_layer != 0)
		{
		    m_WMS->m_Styles += wxT(",");
		    m_WMS->m_Layers += wxT(",");
		}
		m_WMS->m_Styles += style;
		m_WMS->m_Layers += selectedLayers[i_layer]->m_Name;
	}

	m_WMS->m_Proj =  parSettings.Get_Parameter( "PROJ")->asString() ;

	CSG_Parameter_Range * pXrangePar = parSettings.Get_Parameter( "X_RANGE")->asRange();

	m_WMS->m_BBox.xMax =  pXrangePar->Get_HiVal();
	m_WMS->m_BBox.xMin =  pXrangePar->Get_LoVal();

	CSG_Parameter_Range * pYrangePar = parSettings.Get_Parameter( "Y_RANGE")->asRange();

	m_WMS->m_BBox.yMax =  pYrangePar->Get_HiVal();
	m_WMS->m_BBox.yMin =  pYrangePar->Get_LoVal();

	m_WMS->m_Format = parSettings.Get_Parameter( "FORMAT")->asString();

	m_WMS->m_sizeX = parSettings.Get_Parameter("SIZE_X")->asInt();//TODO
	m_WMS->m_sizeY = parSettings.Get_Parameter("SIZE_Y")->asInt();

	m_WMS->m_pOutputMap = Parameters("MAP");

	m_WMS->m_blockSizeX =  parSettings.Get_Parameter("MAX_X")->asInt();
	m_WMS->m_blockSizeY =  parSettings.Get_Parameter("MAX_Y")->asInt();

	m_WMS->modul = this;

	m_WMS->m_bTransparent = parSettings.Get_Parameter("TRANSPARENT")->asBool();

	m_WMS->m_ReProj = parSettings.Get_Parameter("REPROJ_PARAMS")->asString();//TEST na bool
	m_WMS->m_bReProj = parSettings.Get_Parameter("REPROJ")->asBool();
	m_WMS->m_bReProjBbox = parSettings.Get_Parameter("REPROJ_BBOX")->asBool();

	if( parSettings("REPROJ_METHOD")->asString() == SG_T("near")) m_WMS->m_ReProjMethod = 0;
	else if( parSettings("REPROJ_METHOD")->asString() == SG_T("bilinear")) m_WMS->m_ReProjMethod = 1;
	else if( parSettings("REPROJ_METHOD")->asString() == SG_T("cubic")) m_WMS->m_ReProjMethod = 2;
	else if( parSettings("REPROJ_METHOD")->asString() == SG_T("cubicspline")) m_WMS->m_ReProjMethod = 3;
	else if( parSettings("REPROJ_METHOD")->asString() == SG_T("lanczos")) m_WMS->m_ReProjMethod = 4;
	else m_WMS->m_ReProjMethod = 0;


	// downloads map and imports it into SAGA as grid
	try
	{
	    m_WMS->Get_Map();//TODO delete
	}
	catch(WMS_Exception &except)
	{
	    Message_Add((CSG_String("ERROR:") + CSG_String(except.what())));
	    return false;
	}

	return(true);
}


bool CWMS_Import::Create_Layers_Dialog( std::vector<CWMS_Layer*> layers, CSG_Parameters & parLayers)
{
// builds layer tree in dialog
	CWMS_Layer	*	prevLayer;
	CSG_Parameter *	rootLayerPar = NULL;
	CSG_Parameter *	layerPar = NULL;
	CSG_Parameter *	parrentLayerPar = parLayers.Add_Node(NULL, "ND_LAYERS", _TL("Available layers:"), _TL(""));

	for(int i_layer=0; i_layer<layers.size(); i_layer++)
	{
		prevLayer = layers[i_layer];

		layerPar = parLayers.Add_Value(parrentLayerPar, CSG_String::Format(SG_T("layer_%d"), layers[i_layer]->m_id), layers[i_layer]->m_Title, SG_T(""), PARAMETER_TYPE_Bool, false);

		if( i_layer == 0)
		{
			rootLayerPar= layerPar;
			parrentLayerPar = layerPar;
			continue;
		}
		else if( i_layer == layers.size()-1)
		{
			continue;
		}
		else if( layers[i_layer+1]->m_pParrentLayer->m_id == 0)
		{
			parrentLayerPar = rootLayerPar;
			continue;
		}

		bool bPrevIsParrent = (prevLayer->m_id == layers[i_layer+1]->m_pParrentLayer->m_id);
		bool bPrevSameParrent = (layers[i_layer+1]->m_pParrentLayer->m_id == prevLayer->m_pParrentLayer->m_id);

		if(!bPrevIsParrent && !bPrevSameParrent)
		{
			parrentLayerPar = rootLayerPar;
		}

		if( bPrevIsParrent) parrentLayerPar = layerPar;
	     }

	    return true;
    }




bool CWMS_Import::Create_Settings_Dialog( std::vector<CWMS_Layer*> selectedLayers, CSG_Parameters & parSettings)
{
// builds dialog for WMS request settings
	CSG_Parameter * reqPropertiesNode = parSettings.Add_Node(NULL, "ND_REQ_PROPERTIES", _TL("Request properties"), _TL(""));
	CSG_Parameter * projNode = parSettings.Add_Node(reqPropertiesNode, "ND_PROJ", _TL("Projection options"), _TL(""));

	std::vector<wxString> availProjs = m_WMS->m_Capabilities.Proj_Intersect(selectedLayers );

	wxString projChoiceItems;

	for(int j = 0; j< availProjs.size(); j++)
	{
	    projChoiceItems += availProjs[j] + SG_T("|");
	}

	parSettings.Add_Choice( projNode, SG_T("PROJ"), _TL("Choose projection"),_TL("") , projChoiceItems);


	CSG_Parameter * reProjNode = parSettings.Add_Value( projNode, SG_T("REPROJ"), _TL("Reproject downloaded raster"),_TL(""),PARAMETER_TYPE_Bool, false);

	parSettings.Add_String(   reProjNode	, "REPROJ_PARAMS"	, _TL("PROJ 4 format of grid projection:"),    _TL("") ,SG_T("")   );

	parSettings.Add_Value( reProjNode, SG_T("REPROJ_BBOX"), _TL("Coordinates of BBOX in projection of grid:"),_TL(""),PARAMETER_TYPE_Bool, false);

	parSettings.Add_Choice(reProjNode	, "REPROJ_METHOD"	, _TL("Reprojection method")		, _TL(""),
				CSG_String::Format
				(
					SG_T("%s|%s|%s|%s|%s|"),
					SG_T("near"),
					SG_T("bilinear"),
					SG_T("cubic"),
					SG_T("cubicspline"),
					SG_T("lanczos")
				));

	CSG_Parameter * bboxNode = parSettings.Add_Node(reqPropertiesNode, "BBOX", _TL("BBOX"), _TL(""));

	if ((selectedLayers[0]->Get_Projections().size() != 0))//TODO zavislost na vybrane projekci
	{
			CSG_Rect		r(selectedLayers[0]->Get_Projections()[0].m_GeoBBox);
			parSettings.Add_Range	(bboxNode	, "X_RANGE"	, _TL("X Range")	, _TL(""), r.Get_XMin(), r.Get_XMax(), r.Get_XMin(), false, r.Get_XMax(), false);
			parSettings.Add_Range	(bboxNode	, "Y_RANGE"	, _TL("Y Range")	, _TL(""), r.Get_YMin(), r.Get_YMax(), r.Get_YMin(), false, r.Get_YMax(), false);
			parSettings.Add_Value	(bboxNode	, "CELLSIZE", _TL("Cellsize")	, _TL(""), PARAMETER_TYPE_Double, r.Get_XRange() / 2001.0, 0.0, true);//TODO
	}

	parSettings.Add_Choice(reqPropertiesNode	, "FORMAT"	, _TL("Format")		, _TL(""), m_WMS->m_Capabilities.m_Formats);

	parSettings.Add_Value( reqPropertiesNode, SG_T("SIZE_X"), _TL("Requeste image X size:"),_TL(""),PARAMETER_TYPE_Int, 1000);

	parSettings.Add_Value( reqPropertiesNode, SG_T("SIZE_Y"), _TL("Requeste image Y size:"),_TL(""),PARAMETER_TYPE_Int, 1000);


	parSettings.Add_Value( reqPropertiesNode, SG_T("MAX_X"), _TL("Block size X:"),_TL(""),PARAMETER_TYPE_Int, 400);
	parSettings.Add_Value( reqPropertiesNode, SG_T("MAX_Y"), _TL("Block size Y:"),_TL(""),PARAMETER_TYPE_Int, 400);

	//TODO razeni vrstev

	CSG_Parameter * mapStyleNode = parSettings.Add_Node(NULL, "ND_TRANSPARENT", _TL("Map style"), _TL(""));

	parSettings.Add_Value( mapStyleNode, SG_T("TRANSPARENT"), _TL("Request transparent data:"),_TL(""),PARAMETER_TYPE_Bool, true);

	CSG_Parameter * stylesNode = parSettings.Add_Node(mapStyleNode, "ND_STYLES", _TL("Style"), _TL(""));

	for(int i_Layer = 0; i_Layer < selectedLayers.size(); i_Layer++)
	{
		wxString layerStylesCh;
		if( selectedLayers[i_Layer]->Get_Styles().size() > 1)
		{
			for(int j_Style = 0; j_Style< selectedLayers[i_Layer]->Get_Styles().size(); j_Style++)
			{
				layerStylesCh += selectedLayers[i_Layer]->Get_Styles()[j_Style].m_Title + SG_T("|");
			}
			if( layerStylesCh.Length()!=0 )  parSettings.Add_Choice( stylesNode,  wxString::Format(SG_T("style_%d"), (int)selectedLayers[i_Layer]->m_id).c_str(), selectedLayers[i_Layer]->m_Title,_TL(""), layerStylesCh );
		}
	}

	parSettings.Set_Callback_On_Parameter_Changed(_On_Proj_Changed);
	parSettings.Set_Callback(true);

}


int CWMS_Import::_On_Proj_Changed(CSG_Parameter *pParameter, int Flags)
{

}

std::vector<wxString>  CWMS_XmlHandlers::_Get_Children_Content(class wxXmlNode *pNode, const wxString & children_name)
{
	std::vector<wxString> childrenContent;
	wxXmlNode	*pItem	= pNode->GetChildren();

	while( pItem )
	{
		if( pItem->GetName().CmpNoCase(children_name) )
		{
			childrenContent.push_back(pItem->GetNodeContent());
		}
		pItem	= pItem->GetNext();
	}

	return childrenContent;
}

//---------------------------------------------------------
wxXmlNode * CWMS_XmlHandlers::_Get_Child(wxXmlNode *pNode, const wxString &Name)
{
	if( pNode && (pNode = pNode->GetChildren()) != NULL )
	{
		do
		{
			if( !pNode->GetName().CmpNoCase(Name) )
			{
				return( pNode );
			}
		}
		while( (pNode = pNode->GetNext()) != NULL );
	}

	return( NULL );
}

//---------------------------------------------------------
bool CWMS_XmlHandlers::_Get_Child_Content(wxXmlNode *pNode, wxString &Value, const wxString &Name)
{
	if( (pNode = _Get_Child(pNode, Name)) != NULL )
	{
		Value	= pNode->GetNodeContent();

		return( true );
	}

	return( false );
}

//---------------------------------------------------------
bool CWMS_XmlHandlers::_Get_Child_Content(wxXmlNode *pNode, int &Value, const wxString &Name)
{
	long	lValue;

	if( (pNode = _Get_Child(pNode, Name)) != NULL && pNode->GetNodeContent().ToLong(&lValue) )
	{
		Value	= lValue;

		return( true );
	}

	return( false );
}

//---------------------------------------------------------
bool CWMS_XmlHandlers::_Get_Child_Content(wxXmlNode *pNode, double &Value, const wxString &Name)
{
	double	dValue;

	if( (pNode = _Get_Child(pNode, Name)) != NULL && pNode->GetNodeContent().ToDouble(&dValue) )
	{
		Value	= dValue;

		return( true );
	}

	return( false );
}

//---------------------------------------------------------
bool CWMS_XmlHandlers::_Get_Node_PropVal(wxXmlNode *pNode, wxString &Value, const wxString &Property)
{
	wxString	PropVal;

	if( pNode != NULL && pNode->GetPropVal(Property, &PropVal) )
	{
		Value	= PropVal;

		return( true );
	}

	return( false );
}

bool CWMS_XmlHandlers::_Get_Child_PropVal(wxXmlNode *pNode, wxString &Value, const wxString &Name, const wxString &Property)
{
	return( (pNode = _Get_Child(pNode, Name)) != NULL && _Get_Node_PropVal(pNode, Value, Property) );
}
