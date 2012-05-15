#include "wms_import.h"
#include <algorithm>
#include <wx/image.h>


CWMS_Import::CWMS_Import(void)
{

	Set_Name		(_TL("WMS"));

	Set_Author		(SG_T("S. Turek (c) 2012, based on Olaf Conrad's code"));

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

	// new WNS class
    	try
    	{
		m_WMS = new CWMS_Gdal_drv;
    	}
    	catch(std::bad_alloc& except)
    	{
		Message_Add(CSG_String::Format(SG_T("ERROR: Unable to allocate memomry: %s"), except.what()));//TODO translation
		return false;
    	}

    	//Gets information from the first dialog about server URL, password and username
    	m_WMS->m_ServerUrl	= Parameters("SERVER")->asString();

    	if(m_WMS->m_ServerUrl.Contains(wxT("http://")))
    	{
		m_WMS->m_ServerUrl		= Parameters("SERVER")->asString() + 7;
    	}

    	wxString userName = Parameters("USERNAME")->asString();
    	wxString password = Parameters("PASSWORD")->asString();

    	// Gets capabilities data from WMS server
    	try
    	{
		m_WMS->m_Capabilities.Create(m_WMS->m_ServerUrl, userName, password);
    	}
    	catch(CWMS_Exception &except)//TODO CPLEerr
    	{
		Message_Add((CSG_String("ERROR:") + CSG_String(except.what())));
		return false;
    	}
    	catch(std::bad_alloc& except)
    	{
		Message_Add(CSG_String::Format(_TL ("ERROR: Unable to allocate memomry: %s")), except.what());
		return false;
    	}


    	std::vector<CWMS_Layer*> layers;
    	m_WMS->m_Capabilities.Get_Layers(layers);

    	CSG_Parameters	parLayers;
	_Create_Layers_Dialog( layers, parLayers);

	std::vector<CWMS_Layer*> selectedLayers;

	// show layers dialog
	if( Dlg_Parameters(&parLayers, _TL("Choose layers"))) { }

	//get selected layers in the layers dialog
	for(int i = 0; i < layers.size(); i++)
	{
		CSG_Parameter * pLayerPar = parLayers(CSG_String::Format(SG_T("layer_%d"), layers[i]->m_id) );
		if(!pLayerPar) continue;
		if(pLayerPar->asBool())
		{
			selectedLayers.push_back(layers[i]);
		}
	}

	if(selectedLayers.size() < 1)
	{
		Message_Add(_TL("No layers choosen"));
		return false;
	}


	CSG_Parameters	parSettings;
	CSG_Parameters	* pParDialog;

	// dialog with WMS request settings
	pParDialog = _Create_Settings_Dialog(selectedLayers, parSettings);

	// show dialog with WMS request settings
	if( Dlg_Parameters(pParDialog, _TL("WMS Request settings"))) { }


	//puts layers into chosen order

	std::vector<CWMS_Layer*> selectedLayersOrdered(selectedLayers.size(), NULL);
	std::vector<int> ilayerOrderSet;
	for(int i_Layer = 0; i_Layer < selectedLayers.size(); i_Layer++)
	{
		int i = pParDialog->Get_Parameter(CSG_String::Format(SG_T("LAYER_NUMBER_%d"), i_Layer).c_str())->asChoice()->asInt();
		if(i != 0)
		{
			selectedLayersOrdered[i_Layer] = selectedLayers[i - 1];
			ilayerOrderSet.push_back(i - 1);
		}
	}

	//puts not chosen layers (in order layers dialog) into empty places //TODO warning???
	std::vector<CWMS_Layer*>::iterator it;
	for(int i_Layer = 0; i_Layer < selectedLayers.size(); i_Layer++)
	{
		bool bIsSet = false;
		for(int i_ilayerOrderSet = 0; i_ilayerOrderSet < ilayerOrderSet.size(); i_ilayerOrderSet++)
		{
			if(ilayerOrderSet[i_ilayerOrderSet] == i_Layer)
			{
				bIsSet = true;
				break;
			}
		}
		if(bIsSet) continue;

		ilayerOrderSet.push_back(i_Layer);
		for(int i = 0; i < selectedLayersOrdered.size(); i++)
		{
			if(selectedLayersOrdered[i] == NULL)
			{
				selectedLayersOrdered[i] = selectedLayers[i_Layer];
				break;
			}
		}
	}

	// assigns chosen values from pParDialog to m_WMS class members
	wxString style;
	for(int i_Layer = 0; i_Layer < selectedLayersOrdered.size(); i_Layer++)
	{
		CSG_Parameter * pStyleChoicePar = pParDialog->Get_Parameter( wxString::Format(SG_T("STYLE_%d"),  selectedLayersOrdered[i_Layer]->m_id));
		if(pStyleChoicePar)
		{
			style = pStyleChoicePar->asString();
		}
		else style = wxT("");

		if(i_Layer != 0)
		{
		    m_WMS->m_Styles += wxT(",");
		    m_WMS->m_Layers += wxT(",");
		}

		std::vector<wxXmlNode *> selLayersNames = selectedLayersOrdered[i_Layer]->getElms(wxT("Name"));
		if(selLayersNames.size() < 1) continue;

		m_WMS->m_Styles += style;
		m_WMS->m_Layers += selLayersNames[0]->GetNodeContent();
	}

	m_WMS->m_Proj =  pParDialog->Get_Parameter( "PROJ")->asString() ;

	CSG_Parameter_Range * pXrangePar = pParDialog->Get_Parameter( "X_RANGE")->asRange();

	m_WMS->m_BBox.xMax =  pXrangePar->Get_HiVal();
	m_WMS->m_BBox.xMin =  pXrangePar->Get_LoVal();

	CSG_Parameter_Range * pYrangePar = pParDialog->Get_Parameter( "Y_RANGE")->asRange();

	m_WMS->m_BBox.yMax =  pYrangePar->Get_HiVal();
	m_WMS->m_BBox.yMin =  pYrangePar->Get_LoVal();

	m_WMS->m_Format = pParDialog->Get_Parameter( "FORMAT")->asString();

	m_WMS->m_sizeX = pParDialog->Get_Parameter("SIZE_X")->asInt();
	m_WMS->m_sizeY = pParDialog->Get_Parameter("SIZE_Y")->asInt();

	m_pOutputMap = Parameters("MAP");

	m_WMS->m_blockSizeX =  pParDialog->Get_Parameter("MAX_X")->asInt();
	m_WMS->m_blockSizeY =  pParDialog->Get_Parameter("MAX_Y")->asInt();

	m_WMS->m_bTransparent = pParDialog->Get_Parameter("TRANSPARENT")->asBool();

	m_WMS->m_ReProj = pParDialog->Get_Parameter("REPROJ_PARAMS")->asString();
	m_WMS->m_bReProj = pParDialog->Get_Parameter("REPROJ")->asBool();
	m_WMS->m_bReProjBbox = pParDialog->Get_Parameter("REPROJ_BBOX")->asBool();

	CSG_String chosenMethod = pParDialog->Get_Parameter("REPROJ_METHOD")->asString();
	if(chosenMethod == SG_T("near"))		m_WMS->m_ReProjMethod = 0;
	else if(chosenMethod == SG_T("bilinear"))	m_WMS->m_ReProjMethod = 1;
	else if(chosenMethod == SG_T("cubic"))		m_WMS->m_ReProjMethod = 2;
	else if(chosenMethod == SG_T("cubicspline"))	m_WMS->m_ReProjMethod = 3;
	else if(chosenMethod == SG_T("lanczos"))	m_WMS->m_ReProjMethod = 4;
	else						m_WMS->m_ReProjMethod = 0;


	// downloads map and imports it into SAGA as grid

	//TODO version warning
	wxString mapFile;
	try
	{
	    mapFile = m_WMS->Get_Map();
	}
	catch(CWMS_Exception &except)
	{
	    Message_Add((CSG_String("ERROR:") + CSG_String(except.what())));
	    return false;
	}

	if(_Import_Map(mapFile)) return true;
	return false;
}


bool CWMS_Import::_Create_Layers_Dialog(const std::vector<CWMS_Layer *> & layers, CSG_Parameters & parLayers)
{

	CWMS_Layer  * pPrevLayer = NULL;
	CSG_Parameter *	pRootLayerPar = NULL;
	CSG_Parameter *	pLayerPar = NULL;
	CSG_Parameter *	pParrentLayerPar = parLayers.Add_Node(NULL, "ND_LAYERS", _TL("Available layers:"), _TL(""));
	CSG_String nodeTitle;

	for(int i_Layer = 0; i_Layer < layers.size(); i_Layer++)
	{
		pPrevLayer = layers[i_Layer];

		nodeTitle = _Get_Layers_Existing_Name(pPrevLayer);

		if(pPrevLayer->getElms(wxT("Name")).size() == 0)
			pLayerPar = parLayers.Add_Info_String(pParrentLayerPar, CSG_String::Format(SG_T("layer_%d"),
							    layers[i_Layer]->m_id), nodeTitle, SG_T(""), SG_T("") );

		else
			pLayerPar = parLayers.Add_Value(pParrentLayerPar, CSG_String::Format(SG_T("layer_%d"),
						       layers[i_Layer]->m_id), nodeTitle, SG_T(""), PARAMETER_TYPE_Bool, false);

		if(i_Layer == 0)
		{
			pRootLayerPar= pLayerPar;
			pParrentLayerPar = pLayerPar;
			continue;
		}
		else if(i_Layer == layers.size() - 1)
		{
			continue;
		}
		else if(layers[i_Layer+1]->m_pParrentLayer->m_id == 0)
		{
			pParrentLayerPar = pRootLayerPar;
			continue;
		}

		bool bPrevIsParrent = (pPrevLayer->m_id == layers[i_Layer + 1]->m_pParrentLayer->m_id);
		bool bPrevSameParrent = (layers[i_Layer + 1]->m_pParrentLayer->m_id == pPrevLayer->m_pParrentLayer->m_id);

		if(!bPrevIsParrent && !bPrevSameParrent)
		{
			pParrentLayerPar = pRootLayerPar;
		}

		if(bPrevIsParrent) pParrentLayerPar = pLayerPar;
	}

	return true;
   }

CSG_String CWMS_Import::_Get_Layers_Existing_Name(CWMS_Layer * pLayer)
{
	CSG_String layerTitle;
	if(!pLayer) return layerTitle;

	if(pLayer->getElms(wxT("Title")).size() != 0)
		layerTitle = pLayer->getElms(wxT("Title"))[0]->GetNodeContent();
	else if(pLayer->getElms(wxT("Name")).size() != 0)
		layerTitle = pLayer->getElms(wxT("Name"))[0]->GetNodeContent();
	else
		layerTitle = pLayer->m_id;

       return layerTitle;
}

CSG_Parameters * CWMS_Import::_Create_Settings_Dialog(const std::vector<CWMS_Layer*> & selectedLayers, CSG_Parameters & parSettings)
{
 //builds dialog for WMS request settingss

	//get CWMS_Projection (projections and bboxes), which are joint for all selected layers
	std::vector<CWMS_Projection> availProjs = _Proj_Intersect(selectedLayers);

	// in parSettings there are saved data for all projections and bboxes from availProjs, this is used for callback function _Settings_Dial_Callback
	// parSettings also contains parDialog, where are paraneters, which are displayed in dialog
	if (selectedLayers.size() < 1){}
	else if ((selectedLayers[0]->getElms(m_WMS->m_Capabilities.m_ProjTag).size() != 0))
	{
		TSG_Rect bbox;
		CSG_Parameter * pProjDataNode;

		for(int i_Proj = 0; i_Proj < availProjs.size(); i_Proj++)
		{
			pProjDataNode = parSettings.Add_Node( NULL, availProjs[i_Proj].m_Projection, _TL(""),_TL("") );
			bbox = availProjs[i_Proj].m_GeoBBox;
			parSettings.Add_Range	(pProjDataNode	, "X_RANGE", _TL("X Range"), _TL(""), bbox.xMin, bbox.xMax, bbox.xMin, false, bbox.xMax, false);
			parSettings.Add_Range	(pProjDataNode	, "Y_RANGE", _TL("Y Range"), _TL(""), bbox.yMin, bbox.yMax, bbox.yMin, false, bbox.yMax, false);
		}
	}

	// rest of code of the method creates Dialog (parameters in parDialog)
	CSG_Parameters * pParDialog = parSettings.Add_Parameters    (NULL, SG_T("DATA_FOR_CALLBACK"), SG_T(""), SG_T("") )->asParameters();

	pParDialog->Create  (&parSettings, SG_T("DATA_FOR_CALLBACK"), SG_T("DATA_FOR_CALLBACK"), SG_T("DATA_FOR_CALLBACK"));

	CSG_Parameter * pReqPropertiesNode = pParDialog->Add_Node   (NULL, "ND_REQ_PROPERTIES", _TL("Request properties"), _TL(""));


	//------------------------- PROJECTION OPTIONS ---------------------------//

	CSG_Parameter * pProjNode = pParDialog->Add_Node    (pReqPropertiesNode, "ND_PROJ", _TL("Projection options"), _TL(""));

	wxString projChoiceItems;

	for(int j = 0; j< availProjs.size(); j++)
	{
	    projChoiceItems += availProjs[j].m_Projection + SG_T("|");
	}

	CSG_Parameter * pProjChoice = pParDialog->Add_Choice	(pProjNode, SG_T("PROJ"), _TL("Choose projection"),_TL("") , projChoiceItems);

	CSG_Parameter * pReProjNode = pParDialog->Add_Value	(pProjNode, SG_T("REPROJ"), _TL("Reproject downloaded raster"),_TL(""),PARAMETER_TYPE_Bool, false);

	pParDialog->Add_String	(pReProjNode, "REPROJ_PARAMS", _TL("PROJ 4 format of grid projection:"), _TL(""), SG_T(""));

	pParDialog->Add_Value	(pReProjNode, SG_T("REPROJ_BBOX"), _TL("Coordinates of BBOX in projection of grid:"),_TL(""),PARAMETER_TYPE_Bool, false);

	pParDialog->Add_Choice	(pReProjNode	, "REPROJ_METHOD"	, _TL("Reprojection method")		, _TL(""),
				CSG_String::Format
				(
					SG_T("%s|%s|%s|%s|%s|"),
					SG_T("near"),
					SG_T("bilinear"),
					SG_T("cubic"),
					SG_T("cubicspline"),
					SG_T("lanczos")
				));

	CSG_Parameter * pBBoxNode = pParDialog->Add_Node(pReqPropertiesNode, "BBOX", _TL("BBOX"), _TL(""));

	pParDialog->Add_Range(pBBoxNode, "X_RANGE",  _TL("X Range"), _TL(""));
	pParDialog->Add_Range(pBBoxNode, "Y_RANGE",  _TL("Y Range"), _TL(""));

	CSG_Parameter * pBBoxData = parSettings.Get_Parameter(pProjChoice->asString());
	if(pBBoxData)
	{
		pParDialog->Set_Parameter(SG_T("X_RANGE"), pBBoxData->Get_Child(0));
		pParDialog->Set_Parameter(SG_T("Y_RANGE"), pBBoxData->Get_Child(1));
	}


	//-------------------------FORMAT---------------------------//

	wxXmlNode * pReqNode = m_WMS->m_Capabilities.m_XmlH.Get_Child(m_WMS->m_Capabilities.m_pCapNode, SG_T("Request"));
	wxXmlNode * pGetMapNode = m_WMS->m_Capabilities.m_XmlH.Get_Child(pReqNode, SG_T("GetMap"));
	std::vector<wxXmlNode *> formats = m_WMS->m_Capabilities.m_XmlH.Get_Children(pGetMapNode, SG_T("Format"));

	wxString formatsCoice;
	for(int i_Format = 0; i_Format < formats.size(); i_Format++)
	{
		formatsCoice	+= formats[i_Format]->GetNodeContent().c_str();
		formatsCoice	+= SG_T("|");
	}

	pParDialog->Add_Choice(pReqPropertiesNode	, "FORMAT"	, _TL("Format")		, _TL(""), formatsCoice);

	//-------------------------SIZE OPTIONS---------------------------//

	int minSize = 100;
	int maxSize = 20000;

	pParDialog->Add_Value( pReqPropertiesNode, SG_T("SIZE_X"), _TL("Requeste image X size:"),_TL(""),PARAMETER_TYPE_Int, 1000, minSize, true, maxSize, true);

	pParDialog->Add_Value( pReqPropertiesNode, SG_T("SIZE_Y"), _TL("Requeste image Y size:"),_TL(""),PARAMETER_TYPE_Int, 1000, minSize, true, maxSize, true);

	pParDialog->Add_Value( pReqPropertiesNode, SG_T("MAX_X"), _TL("Block size X:"),_TL(""),PARAMETER_TYPE_Int, 400, minSize, true, maxSize, true);

	pParDialog->Add_Value( pReqPropertiesNode, SG_T("MAX_Y"), _TL("Block size Y:"),_TL(""),PARAMETER_TYPE_Int, 400, minSize, true, maxSize, true);


	//-------------------------STYLE OPTIONS---------------------------//

	CSG_Parameter * mapStyleNode = pParDialog->Add_Node(NULL, "ND_STYLE", _TL("Map style"), _TL(""));

	pParDialog->Add_Value( mapStyleNode, SG_T("TRANSPARENT"), _TL("Request transparent data:"),_TL(""),PARAMETER_TYPE_Bool, true);

	CSG_Parameter * stylesNode = pParDialog->Add_Node(mapStyleNode, "ND_STYLES", _TL("Style"), _TL(""));

	wxString layerStylesCh;
	wxString selLayerCh(wxT("|"));
	wxString stTitle, nodeTitle;

	for(int i_Layer = 0; i_Layer < selectedLayers.size(); i_Layer++)
	{
		layerStylesCh.Clear();

		nodeTitle = _Get_Layers_Existing_Name(selectedLayers[i_Layer]);

		if( selectedLayers[i_Layer]->getElms(wxT("Style")).size() > 0)
		{
			for(int j_Style = 0; j_Style< selectedLayers[i_Layer]->getElms(wxT("Style")).size(); j_Style++)
			{
			    if(m_WMS->m_Capabilities.m_XmlH.Get_Child_Content(selectedLayers[i_Layer]->getElms(wxT("Style"))[j_Style], stTitle, wxT("Title")))
			    layerStylesCh += stTitle + SG_T("|");
			}
			if(layerStylesCh.Length()!=0 )  pParDialog->Add_Choice( stylesNode,  wxString::Format(SG_T("STYLE_%d"), (int)selectedLayers[i_Layer]->m_id).c_str(), nodeTitle,_TL(""), layerStylesCh );
		}
		selLayerCh += nodeTitle + SG_T("|");

	}

	//-------------------------LAYERS ORDER---------------------------//

	CSG_Parameter * orderLayerNode = pParDialog->Add_Node(NULL, "ND_LAYER_ORDER", _TL("Layer order"), _TL(""));

	for(int i_Layer = 0; i_Layer < selectedLayers.size(); i_Layer++)
	{
	    pParDialog->Add_Choice(orderLayerNode, CSG_String::Format(SG_T("LAYER_NUMBER_%d"), i_Layer).c_str(), CSG_String::Format(SG_T("Choose layer no. %d"), i_Layer).c_str(), _TL(""), selLayerCh);
	}

	//-------------------------SETTING CALLBACK---------------------------//
	pParDialog->Set_Callback_On_Parameter_Changed(_Settings_Dial_Callback);
	pParDialog->Set_Callback(true);

	return pParDialog;
}


int CWMS_Import::_Settings_Dial_Callback(CSG_Parameter *pParameter, int Flags)
{

	//-------------------------CALLBACK BBOX---------------------------//
	CSG_Parameters * parDialog = pParameter->Get_Owner();
	CSG_Parameters * parSettings =  static_cast<CSG_Parameters*>(parDialog->Get_Owner());

	if(CSG_String(pParameter->Get_Identifier()).Cmp(SG_T("REPROJ")) &&
	   CSG_String(pParameter->Get_Identifier()).Cmp(SG_T("REPROJ_BBOX"))) {}
	else if(parDialog->Get_Parameter("REPROJ")->asBool()  && parDialog->Get_Parameter("REPROJ_BBOX")->asBool())
	{
		CSG_Parameter_Range * rangeX = parDialog->Get_Parameter(SG_T("X_RANGE"))->asRange();
		rangeX->Set_HiVal(0.0);
		rangeX->Set_LoVal(0.0);
		CSG_Parameter_Range * rangeY = parDialog->Get_Parameter(SG_T("Y_RANGE"))->asRange();
		rangeY->Set_HiVal(0.0);
		rangeY->Set_LoVal(0.0);
	}

	if(!CSG_String(pParameter->Get_Identifier()).Cmp(CSG_String("PROJ")) ||
	   !CSG_String(pParameter->Get_Identifier()).Cmp(CSG_String("REPROJ")) ||
	   !CSG_String(pParameter->Get_Identifier()).Cmp(CSG_String("REPROJ_BBOX")))
	{
		CSG_Parameter * projNode = parSettings->Get_Parameter(parDialog->Get_Parameter(SG_T("PROJ"))->asString());
		if(projNode)
		{
			parDialog->Set_Parameter(SG_T("X_RANGE"), projNode->Get_Child(0));
			parDialog->Set_Parameter(SG_T("Y_RANGE"), projNode->Get_Child(1));
		}
	}

	//-------------------------CALLBACK LAYERS ORDER---------------------------//

	CSG_Parameter * parent = pParameter->Get_Parent();
	if(!parent) return 1;

	if(!CSG_String(parent->Get_Identifier()).Cmp(CSG_String("ND_LAYER_ORDER")))
	{
	    CSG_Parameter_Choice * pChoice;
	    std::vector<CSG_String> items;
	    std::vector<int> alreadyChosenItems;
	    CSG_String choosenItem, choiceItems;
	    int choosenItemNum;

	    for (int i_Choice = 0; i_Choice < parent->Get_Children_Count(); i_Choice++)
	    {
		pChoice =  parent->Get_Child(i_Choice)->asChoice();
		if(i_Choice == 0)
		{
			for(int i_Item = 0; i_Item < pChoice->Get_Count(); i_Item++)
			{
				items.push_back(CSG_String(pChoice->Get_Item(i_Item)));
			}
		}

		choosenItem =  parent->Get_Child(i_Choice)->asString();
		choosenItemNum = -1;

		for(int i_Item = 0; i_Item < items.size(); i_Item++)
		{
			if(!choosenItem.Cmp(items[i_Item]))
			{
				choosenItemNum = i_Item;
				break;
			}
		}

		choiceItems.Clear();
		bool bSetEmpty = true;

		for(int i_Item = 0; i_Item < items.size(); i_Item++)
		{
			if(std::find(alreadyChosenItems.begin(), alreadyChosenItems.end(), i_Item) == alreadyChosenItems.end())
			{
				choiceItems += items[i_Item] + SG_T("|");
				if(i_Item == choosenItemNum) bSetEmpty = false;
			}
		}

		pChoice->Set_Items(choiceItems);

		bSetEmpty?pChoice->CSG_Parameter_Int::Set_Value(0):pChoice->Set_Value((SG_Char*)(items[choosenItemNum].c_str()));

		if(choosenItemNum != 0) alreadyChosenItems.push_back(choosenItemNum);
	    }
	}
}

std::vector<CWMS_Projection> CWMS_Import::_Proj_Intersect(const std::vector<CWMS_Layer*> & layers)
{
	//-------------------------JOINT PROJECTIONS FOR SELECTED LAYERS---------------------------//

    	CWMS_Projection proj;
	std::vector<CWMS_Projection> projs;
	std::vector<wxXmlNode *> intersProjs, intersProjsTemp, intersBboxes, layerProjs;

	if(layers.size() > 0)
	{
		   intersProjs = layers[0]->getElms(m_WMS->m_Capabilities.m_ProjTag);
	}


	for(int i_Layer = 1; i_Layer < layers.size(); i_Layer++)
	{
		intersProjsTemp.clear();
		layerProjs = layers[i_Layer]->getElms(m_WMS->m_Capabilities.m_ProjTag);

		for(int i_LayerProj = 0; i_LayerProj < layerProjs.size(); i_LayerProj++ )
		{
			bool isInside = false;
			for(int i_InterProj = 0; i_InterProj < intersProjs.size(); i_InterProj++ )
			{
				if(!layerProjs[i_LayerProj]->GetNodeContent().Cmp(intersProjs[i_InterProj]->GetNodeContent()))
				{
					isInside = true;
					break;
				}
			}
			if(isInside) intersProjsTemp.push_back(layerProjs[i_LayerProj]);
		}
		intersProjs = intersProjsTemp;
	}

	for(int i_Proj = 0; i_Proj < intersProjs.size(); i_Proj++ )
	{
		if(intersProjs[i_Proj]->GetNodeContent().IsEmpty()) continue;
		proj.m_Projection = intersProjs[i_Proj]->GetNodeContent();
		projs.push_back(proj);

	}

	//-------------------------SERCH OF BBOX FOR EVERY PROJECTION---------------------------//

	std::vector<wxXmlNode *> layerBboxes;

	for(int i_Proj = 0; i_Proj < projs.size(); i_Proj++)
	{
		for(int i_Layer = 0; i_Layer < layers.size(); i_Layer++)
		{
			layerBboxes = layers[i_Layer]->getElms(wxT("BoundingBox"));

			for(int i_Bbox = 0; i_Bbox < layerBboxes.size(); i_Bbox++ )
			{
				wxString s;
				if(!layerBboxes[i_Bbox]->GetPropVal(m_WMS->m_Capabilities.m_ProjTag, &s))
					continue;
				if(s.CmpNoCase(projs[i_Proj].m_Projection))
					continue;

				if(layerBboxes[i_Bbox]->GetPropVal(wxT("minx"), &s))
					s.ToDouble(&projs[i_Proj].m_GeoBBox.xMin);
				if(layerBboxes[i_Bbox]->GetPropVal(wxT("miny"), &s))
					s.ToDouble(&projs[i_Proj].m_GeoBBox.yMin);
				if(layerBboxes[i_Bbox]->GetPropVal(wxT("maxx"), &s))
					s.ToDouble(&projs[i_Proj].m_GeoBBox.xMax);
				if(layerBboxes[i_Bbox]->GetPropVal(wxT("maxy"), &s))
					s.ToDouble(&projs[i_Proj].m_GeoBBox.yMax);
		     }
		}
	}
	return projs;
}


bool	CWMS_Import::_Import_Map(const wxString & tempMapPath)
{

	    wxImage	Image;
	    if( Image.LoadFile(tempMapPath) == false )
	    {
		Message_Add(_TL("Unamble to load file with fetched data."));
		return false;

	    }

	    else
	    {
		    CSG_Grid	*pGrid	= SG_Create_Grid(SG_DATATYPE_Int, Image.GetWidth(), Image.GetHeight(), 1, m_WMS->m_BBox.xMin, m_WMS->m_BBox.yMin);

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

		    CSG_String gridName =  _Get_Layers_Existing_Name(m_WMS->m_Capabilities.m_RootLayer);
		    pGrid->Set_Name(gridName);

		    m_pOutputMap->Set_Value(pGrid);
		    DataObject_Set_Colors(pGrid, 100, SG_COLORS_BLACK_WHITE);
		    CSG_Parameters Parameters;

		    if(DataObject_Get_Parameters(pGrid, Parameters) && Parameters("COLORS_TYPE") )
		    {
			    Parameters("COLORS_TYPE")->Set_Value(3);	// Color Classification Type: RGB
			    DataObject_Set_Parameters(pGrid, Parameters);
		    }
		    if(!m_WMS->m_LayerProjDef.IsEmpty())pGrid->Get_Projection().Create((CSG_String) m_WMS->m_LayerProjDef, SG_PROJ_FMT_Proj4);

	}
	wxRemoveFile(tempMapPath);
	return true;
}
