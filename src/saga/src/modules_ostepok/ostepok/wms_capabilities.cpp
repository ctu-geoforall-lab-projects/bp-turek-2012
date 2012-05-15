#include "wms_import.h"

CWMS_Capabilities::CWMS_Capabilities(void)
{
	m_RootLayer = NULL;
}


CWMS_Capabilities::~CWMS_Capabilities(void)
{
	_Reset();
}

void CWMS_Capabilities::_Reset(void)
{
	if(m_RootLayer)
	{
	    m_RootLayer->_Delete_Child_Layers();
	    SG_Free(m_RootLayer);
	}

	m_pCapNode = NULL;
	m_pServiceNode = NULL;
	m_Version.Clear();
	m_ProjTag.Clear();
}

void CWMS_Capabilities::Create(const wxString & serverUrl, const wxString & userName, const wxString & password)
{
	_Reset();
	wxHTTP server;
	wxString serverUrlAdress = serverUrl.BeforeFirst(SG_T('/'));
	server.SetUser(userName.c_str());
	server.SetPassword(password.c_str());

	if(server.Connect(serverUrlAdress.c_str()) == false)
	{
		throw CWMS_Exception(wxT("ERROR: Unable to connect to server."));
	}


    	wxString	serverDirectory = wxT("/") + serverUrl.AfterFirst(wxT('/'));
    	wxString	getCapRequest(serverDirectory);

    	getCapRequest	+= wxT("?SERVICE=WMS");
    	getCapRequest	+= wxT("&VERSION=1.1.1");
    	getCapRequest	+= wxT("&REQUEST=GetCapabilities");

    	//-------------------------------------------------
    	wxInputStream	*pStream;
    	if( (pStream = server.GetInputStream(getCapRequest.c_str())) != NULL)
    	{

		if(!m_XmlTree.Load(*pStream))
		{
			throw CWMS_Exception(wxT("Unable to load and parse GetCapabilities xml document."));
		}
		_InitializeCapabilities(m_XmlTree.GetRoot());

    	}
    	else
    	{
		throw CWMS_Exception(wxT("Unable to get input stream."));
    	}

}



void CWMS_Capabilities::_InitializeCapabilities(wxXmlNode *pRoot)
{
//Parses servis tag and format in capabilities tag, then  calls _Load_Layers function

	wxXmlNode * pNodeRootLayer;
	wxXmlNode * pNodeCap;

	if(!pRoot->GetPropVal(wxT("version"), &m_Version))
	{
		throw CWMS_Exception(wxT("Unable to find Version attribute in GetCapabilities root tag."));
	}

	if (!m_Version.Cmp(wxT("1.3.0")))	m_ProjTag = wxT("CRS");
	else					m_ProjTag = wxT("SRS");


	m_pServiceNode =  m_XmlH.Get_Child(pRoot, wxT("Service"));
	if(m_pServiceNode == NULL)
	{
		throw CWMS_Exception(wxT("Unable to find <Service> tag in GetCapabilities response file."));
	}

	m_pCapNode = m_XmlH.Get_Child(pRoot, wxT("Capability"));
	if(m_pCapNode == NULL)
	{
		throw CWMS_Exception(wxT("Unable to find <Capaility> tag in GetCapabilities response file."));;
	}


	if((pNodeRootLayer = m_XmlH.Get_Child(m_pCapNode, wxT("Layer"))) == NULL)
	{
		throw CWMS_Exception(wxT("Unanle to find any <Layer> tag in GetCapabilities response file."));
	}

	//Initial value of layer id, incremented when some layer is added.
	int id = 0;
	_InitializeLayerTree(pNodeRootLayer, id);

}


CWMS_Layer * CWMS_Capabilities::_InitializeLayerTree(class wxXmlNode *pLayerNode, int & layerId, CWMS_Layer *pParrentLayer, int nDepth)
{
//Recursive method, which goes down through layer tree, every call establishes new CWMS_Layer object and calls create method of the object to parse atttribites of layer

	CWMS_Layer * pChildLayer;
	CWMS_Layer * pLayer	= new CWMS_Layer;

	pLayer->Create(pLayerNode, pParrentLayer, layerId,  &m_XmlH, m_ProjTag );

	if(nDepth == 0)
	{
		m_RootLayer = pLayer;
	}
	nDepth++;

	std::vector<wxXmlNode *> children = m_XmlH.Get_Children(pLayerNode, wxT("Layer"));
	for(int i_Child = 0; i_Child < children.size(); i_Child++)
	{
		pChildLayer = _InitializeLayerTree(children[i_Child], ++layerId, pLayer, nDepth);
		pLayer->m_ChildLayers.push_back(pChildLayer);
	}

	return pLayer;
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

	for(int i = 0; i < layer->m_ChildLayers.size(); i++)
	{
		layers.push_back(layer->m_ChildLayers[i]);
		Get_Layers(layers, layer->m_ChildLayers[i]);
	}

}



