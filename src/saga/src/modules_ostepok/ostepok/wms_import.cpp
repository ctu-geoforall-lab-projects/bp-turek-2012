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



///////////////////////////////////////////////////////////
//														 //
//														 //
//														 //
///////////////////////////////////////////////////////////

//---------------------------------------------------------

#include "wms_import.h"
#include <wx/xml/xml.h>



///////////////////////////////////////////////////////////
//														 //
//														 //
//														 //
///////////////////////////////////////////////////////////

//---------------------------------------------------------






///////////////////////////////////////////////////////////
//														 //
//														 //
//														 //
///////////////////////////////////////////////////////////

//---------------------------------------------------------
CWMS_Import::CWMS_Import(void)
{
	Set_Name		(_TL(" WMS"));

	Set_Author		(SG_T("O. Conrad (c) 2008"));

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
		_TL("Zkouska                        rgrgg   "),
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


///////////////////////////////////////////////////////////
//														 //
//														 //
//														 //
///////////////////////////////////////////////////////////

//---------------------------------------------------------
bool CWMS_Import::On_Execute(void)
{

	m_WMS = new CWMS_WMS_Gdal_drv;
	//-----------------------------------------------------
	m_WMS->m_ServerUrl	= Parameters("SERVER")->asString();

	if( m_WMS->m_ServerUrl.Contains(SG_T("http://")) )
	{
		m_WMS->m_ServerUrl		= Parameters("SERVER")->asString() + 7;
	}

	m_WMS->m_Directory = SG_T("/") + m_WMS->m_ServerUrl.AfterFirst(SG_T('/'));
	CSG_String ServerUrl	= m_WMS->m_ServerUrl.BeforeFirst(SG_T('/'));


	//-----------------------------------------------------
	m_WMS->m_Capabilities.m_Server.SetUser		(Parameters("USERNAME")->asString());
	m_WMS->m_Capabilities.m_Server.SetPassword	(Parameters("PASSWORD")->asString());

	if( m_WMS->m_Capabilities.m_Server.Connect(ServerUrl.c_str()) == false )
	{
		Message_Add(_TL("Unable to connect to server."));

		return( false );
	}



	//-----------------------------------------------------
	if( m_WMS->m_Capabilities.Create(m_WMS) == false )
	{
		Message_Add(_TL("Unable to get capabilities."));

		return( false );
	}

	std::vector<CWMS_Layer*> layers;

	m_WMS->m_Capabilities.Get_Layers( m_WMS->m_Capabilities.m_RootLayer, layers );

	for(int i = 0; i<layers.size(); i++)
	{
		Message_Add( layers[i]->m_Name );

		Message_Add( CSG_String::Format(SG_T("Column %d"), (int) layers[i]->Get_Projections().size())) ;

		Message_Add(   layers[i]->Get_Projections()[0].m_Projection) ;

		for(int j = 0; j< layers[i]->Get_Projections().size(); j++)
		{

		    Message_Add(layers[i]->Get_Projections()[j].m_Projection);
		}

	}



	 CSG_Parameters	parLayers;

	 Create_Layers_Dialog( layers, parLayers);


		//-----------------------------------------------------

	if( Dlg_Parameters(&parLayers, _TL("WMS Import"))) { }//TODO

	CSG_Parameters	parSettings;


	std::vector<CWMS_Layer*> selectedLayers;
	for(int i=0; i<layers.size(); i++)
	{
	       if( parLayers(CSG_String::Format(SG_T("layer_%d"), layers[i]->m_id) )->asBool() )
		{
			//if( n++ > 0 )	Layers	+= SG_T(",");
			selectedLayers.push_back(layers[i]);
		}
	}
	if(selectedLayers.size() == 0) return false; //TODO




	Create_Settings_Dialog(selectedLayers, parSettings);


	if( Dlg_Parameters(&parSettings, _TL("WMS Import"))) { }

	CSG_String style;


	for(int i_layer = 0; i_layer < selectedLayers.size(); i_layer++)
	{

		CSG_Parameter * pStyleChoicePar = parSettings.Get_Parameter( CSG_String::Format(SG_T("style_%d"),  selectedLayers[i_layer]->m_id));
		if(pStyleChoicePar)
		{
			style = pStyleChoicePar->asString();
		}
		else style = "";
		if(i_layer != 0)
		{
		    m_WMS->m_Styles += ",";
		    m_WMS->m_Layers += ",";
		}
		m_WMS->m_Styles += style;//TODO test prazdnych style
		m_WMS->m_Layers += selectedLayers[i_layer]->m_Name;
	}

	m_WMS->m_Proj = parSettings.Get_Parameter( "PROJ")->asString();

	CSG_Parameter_Range * pXrangePar = parSettings.Get_Parameter( "X_RANGE")->asRange();

	m_WMS->m_BBox.xMax =  pXrangePar->Get_HiVal();
	m_WMS->m_BBox.xMin =  pXrangePar->Get_LoVal();

	CSG_Parameter_Range * pYrangePar = parSettings.Get_Parameter( "Y_RANGE")->asRange();

	m_WMS->m_BBox.yMax =  pYrangePar->Get_HiVal();
	m_WMS->m_BBox.yMin =  pYrangePar->Get_LoVal();

	m_WMS->m_Format = parSettings.Get_Parameter( "FORMAT")->asString();

	m_WMS->m_sizeX = 1000;//TODO
	m_WMS->m_sizeY = 1000;
	m_WMS->m_pOutputMap = Parameters("MAP");

	m_WMS->m_blockSizeX = 1000;//TODO
	m_WMS->m_blockSizeY = 600;

	m_WMS->modul = this;

	m_WMS->m_ReProj = parSettings.Get_Parameter("SOURCE_PROJ")->asString();
	m_WMS->m_bReProj = parSettings.Get_Parameter("REPROJ")->asBool();


	m_WMS->Get_Map();//TODO delete
	return( true );
}


bool CWMS_Import::Create_Layers_Dialog( std::vector<CWMS_Layer*> layers, CSG_Parameters & parLayers)
{

    CWMS_Layer	*    prevLayer;
    CSG_Parameter	*    rootParameter = NULL;
    CSG_Parameter	*    tempParameter = NULL;
    CSG_Parameter	*    parameter = NULL;

    for(int i_layer=0; i_layer<layers.size(); i_layer++)
    {
	i_layer == 0 ? prevLayer = layers[0] : prevLayer = layers[i_layer - 1];

	if( i_layer == 0 )//TODO zjednudusit na jednu?
	{
		rootParameter = parLayers.Add_Value(parameter, CSG_String::Format(SG_T("layer_%d"), layers[i_layer]->m_id), layers[i_layer]->m_Title, SG_T(""), PARAMETER_TYPE_Bool, false);
		continue;
	}


	else if( layers[i_layer]->m_pParrentLayer->m_id == 0 )
	{
		tempParameter = parLayers.Add_Value(rootParameter, CSG_String::Format(SG_T("layer_%d"), layers[i_layer]->m_id), layers[i_layer]->m_Title, SG_T("Abstract"), PARAMETER_TYPE_Bool, false);
		parameter = rootParameter;
		continue;
	}

	bool bPrevIsParrent = (prevLayer->m_id == layers[i_layer]->m_pParrentLayer->m_id);
	bool bPrevSameParrent = (layers[i_layer]->m_pParrentLayer->m_id == prevLayer->m_pParrentLayer->m_id);

	if(!bPrevIsParrent && !bPrevSameParrent)
	{
		parameter = rootParameter;
	}

	if( bPrevIsParrent ) parameter = tempParameter;


	tempParameter = parLayers.Add_Value(parameter, CSG_String::Format(SG_T("layer_%d"), layers[i_layer]->m_id) , layers[i_layer]->m_Title, SG_T(""), PARAMETER_TYPE_Bool, false);
	parLayers.Set_Callback_On_Parameter_Changed(_On_Layer_Changed);

     }

    return true;


}


int CWMS_Import::_On_Layer_Changed(CSG_Parameter *pParameter, int Flags)
{

//TODO
}

bool CWMS_Import::Create_Settings_Dialog( std::vector<CWMS_Layer*> selectedLayers, CSG_Parameters & parSettings)
{

    CSG_Parameter * projNode = parSettings.Add_Node(NULL, "PROJ_NODE", _TL("Projection options"), _TL(""));

    CSG_Strings AvailProjs = m_WMS->m_Capabilities.Proj_Intersect(selectedLayers );

    CSG_String layerProjCh;
    for(int j = 0; j< AvailProjs.Get_Count(); j++)
    {

	layerProjCh += AvailProjs[j] + SG_T("|");

    }


    parSettings.Add_Choice( projNode, SG_T("PROJ"), _TL("Choose projection"),_TL("") , layerProjCh);
    if ((selectedLayers[0]->Get_Projections().size() != 0))//TODO zavislost na vybrane projekci
    {
	    CSG_Rect		r(selectedLayers[0]->Get_Projections()[0].m_GeoBBox);
	    parSettings.Add_Range	(projNode	, "X_RANGE"	, _TL("X Range")	, _TL(""), r.Get_XMin(), r.Get_XMax(), r.Get_XMin(), false, r.Get_XMax(), false);
	    parSettings.Add_Range	(projNode	, "Y_RANGE"	, _TL("Y Range")	, _TL(""), r.Get_YMin(), r.Get_YMax(), r.Get_YMin(), false, r.Get_YMax(), false);

	    parSettings.Add_Value	(projNode	, "CELLSIZE", _TL("Cellsize")	, _TL(""), PARAMETER_TYPE_Double, r.Get_XRange() / 2001.0, 0.0, true);

    }

    CSG_Parameter * reProjNode = parSettings.Add_Value( projNode, SG_T("REPROJ"), _TL("Reproject raster to projection (include bbox)"),_TL(""),PARAMETER_TYPE_Bool, false);

	    parSettings.Add_String(   reProjNode	, "SOURCE_PROJ"	, _TL("Source Projection Parameters"),    _TL("") ,SG_T("")   );


    //TODO razeni vrstev, transp.....
    parSettings.Add_Choice(NULL	, "FORMAT"	, _TL("Format")		, _TL(""), m_WMS->m_Capabilities.m_Formats);


    CSG_Parameter * stylesNode = parSettings.Add_Node(NULL, "STYLES", _TL("Style"), _TL(""));

    for(int i_Layer = 0; i_Layer < selectedLayers.size(); i_Layer++)
    {
	CSG_String layerStylesCh;
	if( selectedLayers[i_Layer]->Get_Styles().size() >= 0)
	    {
		for(int j_Style = 0; j_Style< selectedLayers[i_Layer]->Get_Styles().size(); j_Style++)
		{
		    layerStylesCh += selectedLayers[i_Layer]->Get_Styles()[j_Style].m_Title + SG_T("|");//TODO 1 style dont print
		}

		if( layerStylesCh.Length()!=0 )  parSettings.Add_Choice( stylesNode,  CSG_String::Format(SG_T("style_%d"), (int)selectedLayers[i_Layer]->m_id).c_str(), selectedLayers[i_Layer]->m_Title,_TL(""), layerStylesCh );
		    //selected_layers.push_back(layers[i]);
	    }
    }

}




///////////////////////////////////////////////////////////
//														 //
//														 //
//														 //
///////////////////////////////////////////////////////////

CSG_Strings  CWMS_XmlHandlers::_Get_Children_Content(class wxXmlNode *pNode, const CSG_String & children_name)
{
	CSG_Strings childrenContent;
	wxXmlNode	*pItem	= pNode->GetChildren();

	while( pItem )
	{
		if( pItem->GetName().CmpNoCase(children_name) )
		{
			childrenContent.Add(pItem->GetNodeContent().c_str());
		}
		pItem	= pItem->GetNext();
	}

	return childrenContent;
}

//---------------------------------------------------------
wxXmlNode * CWMS_XmlHandlers::_Get_Child(wxXmlNode *pNode, const CSG_String &Name)
{
	if( pNode && (pNode = pNode->GetChildren()) != NULL )
	{
		do
		{
			if( !pNode->GetName().CmpNoCase(Name.c_str()) )
			{
				return( pNode );
			}
		}
		while( (pNode = pNode->GetNext()) != NULL );
	}

	return( NULL );
}

//---------------------------------------------------------
bool CWMS_XmlHandlers::_Get_Child_Content(wxXmlNode *pNode, CSG_String &Value, const CSG_String &Name)
{
	if( (pNode = _Get_Child(pNode, Name)) != NULL )
	{
		Value	= pNode->GetNodeContent();

		return( true );
	}

	return( false );
}

//---------------------------------------------------------
bool CWMS_XmlHandlers::_Get_Child_Content(wxXmlNode *pNode, int &Value, const CSG_String &Name)
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
bool CWMS_XmlHandlers::_Get_Child_Content(wxXmlNode *pNode, double &Value, const CSG_String &Name)
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
bool CWMS_XmlHandlers::_Get_Node_PropVal(wxXmlNode *pNode, CSG_String &Value, const CSG_String &Property)
{
	wxString	PropVal;

	if( pNode != NULL && pNode->GetPropVal(Property.c_str(), &PropVal) )
	{
		Value	= PropVal.c_str();

		return( true );
	}

	return( false );
}

bool CWMS_XmlHandlers::_Get_Child_PropVal(wxXmlNode *pNode, CSG_String &Value, const CSG_String &Name, const CSG_String &Property)
{
	return( (pNode = _Get_Child(pNode, Name)) != NULL && _Get_Node_PropVal(pNode, Value, Property) );
}
