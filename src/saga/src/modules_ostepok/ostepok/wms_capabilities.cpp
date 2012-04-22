#include "wms_import.h"
#include <wx/xml/xml.h>



CWMS_Capabilities::CWMS_Capabilities(void)
{

	_Reset();
}

CWMS_Capabilities::~CWMS_Capabilities(void)
{}

void CWMS_Capabilities::_Reset(void)
{
	m_MaxLayers		= -1;
	m_MaxWidth		= -1;
	m_MaxHeight		= -1;

	m_Name			.Clear();
	m_Title			.Clear();
	m_Abstract		.Clear();
	m_Online		.Clear();
	m_Contact		.Clear();
	m_Fees			.Clear();
	m_Access		.Clear();
	m_Formats		.Clear();
	m_ProjTag		.Clear();
	m_Version               .Clear();

	m_Keywords		.clear();
	m_Formats		.Clear();
	m_RootLayer = 		NULL;

	if(m_RootLayer) m_RootLayer->_Delete_Child_Layers();
	SG_Free(m_RootLayer);
}


void CWMS_Capabilities::Create(CWMS_WMS_Base * pParrentWMS)
{
//Send GetCapabilities request to WMS server, fetch response, and calls _GetCapabilities method.
	_Reset();

	m_ParrentWMS = pParrentWMS;

	wxString	serverDirectory = wxT("/") + m_ParrentWMS->m_ServerUrl.AfterFirst(wxT('/'));
	wxString	getCapRequest(serverDirectory);

	getCapRequest	+= wxT("?SERVICE=WMS");
	getCapRequest	+= wxT("&VERSION=1.1.1");
	getCapRequest	+= wxT("&REQUEST=GetCapabilities");

	//-------------------------------------------------
	wxInputStream	*pStream;

	if( (pStream = m_Server.GetInputStream(getCapRequest.c_str())) != NULL)
	{
		wxXmlDocument	Capabilities;

		if(Capabilities.Load(*pStream))
		{
			// parsing GetCapabilities file into internal data structure
			_Get_Capabilities(Capabilities.GetRoot());
		}
		else
		{
		    throw WMS_Exception(wxT("Unable to load and parse GetCapabilities xml document."));
		}
		delete(pStream);
	}
	else
	{
	    throw WMS_Exception(wxT("Unable to get input stream."));
	}

}



void CWMS_Capabilities::_Get_Capabilities(wxXmlNode *pRoot)
{
//Parses servis tag and format in capabilities tag, then  calls _Load_Layers function

	wxXmlNode	*pNode, *pChild;

	// 1. Parsing Service Tag
	CWMS_XmlHandlers::_Get_Node_PropVal(pRoot, m_Version, wxT("version"));//TODO version warning?

	if (!m_Version.Cmp(wxT("1.3.0")))	m_ProjTag = wxT("CRS");
	else					m_ProjTag = wxT("SRS");


	if( (pNode = CWMS_XmlHandlers::_Get_Child(pRoot, wxT("Service"))) == NULL)
	{
	    throw WMS_Exception(wxT("Unable to find <Service> tag in GetCapabilities response file."));
	}

	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Name		, wxT("Name"));//TODO cyklus
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Title		, wxT("Title"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Abstract		, wxT("Abstract"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Fees		, wxT("Fees"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Access		, wxT("AccessConstraints"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MaxLayers		, wxT("LayerLimit"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MaxWidth		, wxT("MaxWidth"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MaxHeight		, wxT("MaxHeight"));
	CWMS_XmlHandlers::_Get_Child_PropVal(pNode, m_Online		, wxT("OnlineResource"), wxT("xlink:href"));

	if( (pChild = CWMS_XmlHandlers::_Get_Child(pNode, wxT("KeywordList"))) != NULL)
	{
		m_Keywords = CWMS_XmlHandlers::_Get_Children_Content(pNode,wxT("keyword"));
	}

	if( (pChild = CWMS_XmlHandlers::_Get_Child(pNode, wxT("ContactInformation"))) != NULL)//TODO
	{
	}

	// 2. Capabilities

	if((pNode = CWMS_XmlHandlers::_Get_Child(pRoot, wxT("Capability"))) == NULL)
	{
		throw WMS_Exception(wxT("Unable to find <Capaility> tag in GetCapabilities response file."));;
	}


	// 2.a) Request

	if( (pChild = CWMS_XmlHandlers::_Get_Child(CWMS_XmlHandlers::_Get_Child(CWMS_XmlHandlers::_Get_Child(pNode, wxT("Request")), wxT("GetMap")), wxT("Format"))) != NULL )//TODO 1.0
	{
		do
		{
			if( !pChild->GetName().CmpNoCase(wxT("Format")) )
			{
				m_Formats	+= pChild->GetNodeContent().c_str();
				m_Formats	+= wxT("|");
			}
		}
		while( (pChild = pChild->GetNext()) != NULL );
	}


	// 2.b) Exception, Vendor Specific Capabilities, User Defined Symbolization, ...


	// 2.c) Layers


	if( (pNode = CWMS_XmlHandlers::_Get_Child(pNode, wxT("Layer"))) == NULL)
	{
	    throw WMS_Exception(wxT("Unanle to find any <Layer> tag in GetCapabilities response file."));
	}

	//Initial value of layer id, incremented when some layer is added.
	int id = 0;

	_Load_Layers(pNode, id);

}


void  CWMS_Capabilities:: _Load_Layers(class wxXmlNode *pLayerNode, int & layerId, CWMS_Layer *pParrentLayer, int nDepth)
{    
//Recursive method, which goes down through layer tree, every call establishes new CWMS_Layer object and calls create method of the object to parse atttribites of layer

	wxXmlNode   *pChild;

	CWMS_Layer	*pLayer	= new CWMS_Layer;
	pLayer->Create(pLayerNode, pParrentLayer, layerId, m_ProjTag );

	if(nDepth == 0)
	{
		m_RootLayer = pLayer;

	}
	nDepth++;

	pChild = pLayerNode->GetChildren();
	do
	{
		if(!pChild->GetName().CmpNoCase(wxT("Layer")))
		{

			_Load_Layers(pChild, ++layerId,pLayer, nDepth);
		}
	}
	while((pChild = pChild->GetNext()) != NULL);
}


void CWMS_Capabilities::Get_Layers(std::vector<CWMS_Layer*> & layers, CWMS_Layer * layer)
{
// recursive function, returns all layers in tree in vector of pointers

	if(layers.empty())
	{
		if(m_RootLayer)
		{
		    layer = m_RootLayer;
		    layers.push_back(m_RootLayer);
		}
		else return;
	}
	for(int i = 0; i < layer->m_LayerChildren.size(); i++)
	{
		layers.push_back( layer->m_LayerChildren[i] );
		Get_Layers(layers, layer->m_LayerChildren[i]);
	}

}


std::vector<wxString> CWMS_Capabilities::Proj_Intersect(std::vector<CWMS_Layer*> & layers  )
{
	std::vector<wxString> intersProjs;
	std::vector<wxString> intersProjsTemp;

	if(layers.size() > 0)
	{
		for(int i_proj = 0; i_proj <  layers[0]->Get_Projections().size(); i_proj++ )
		{
			intersProjs.push_back(layers[0]->Get_Projections()[i_proj].m_Projection);
		}
	}

	for(int i_Num = 1; i_Num < layers.size(); i_Num++)
	{

		intersProjsTemp.clear();
		std::vector<class CWMS_Projection> projections = layers[i_Num]->Get_Projections();

		for(int i_proj = 0; i_proj < projections.size(); i_proj++ )
		{
			bool isInside =false;
			for(int i_intersecPproj1 = 0; i_intersecPproj1 < intersProjs.size(); i_intersecPproj1++ )
			{
				if( !projections[i_proj].m_Projection.Cmp(intersProjs[i_intersecPproj1]) )
				{
					isInside = true;
					break;
				}
			}
			if(isInside) intersProjsTemp.push_back( projections[i_proj].m_Projection );
		}
		intersProjs = intersProjsTemp;
	}

	return intersProjs;
}

