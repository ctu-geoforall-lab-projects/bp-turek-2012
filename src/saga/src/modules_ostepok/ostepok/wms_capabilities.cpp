#include <wx/xml/xml.h>
#include <wx/image.h>

#include "wms_import.h"



#define V_SRS(Version)	(Version.Contains(SG_T("1.3")) ? SG_T("CRS") : SG_T("SRS"))
#define S_SRS(Version)	(Version.Cmp(SG_T("1.3.0")) ? SG_T("&SRS=") : SG_T("&CRS="))

#define V_MAP(Version)	(Version.Cmp(SG_T("1.0.0")) ? SG_T("GetMap") : SG_T("Map"))//TODO


CWMS_Capabilities::CWMS_Capabilities(void)
{

	_Reset();
}

CWMS_Capabilities::~CWMS_Capabilities(void)
{}

//---------------------------------------------------------
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
	m_Keywords		.Clear();

	m_Formats		.Clear();
	m_RootLayer = 		NULL;

	if(m_RootLayer) m_RootLayer->_Delete_Child_Layers();
	SG_Free(m_RootLayer);
}


//---------------------------------------------------------
bool CWMS_Capabilities::Create(CWMS_WMS_Base * pParrentWMS)
{
	bool	bResult	= false;

	_Reset();

	m_ParrentWMS = pParrentWMS;


	CSG_String	sRequest(pParrentWMS->m_Directory);

	sRequest	+= SG_T("?SERVICE=WMS");
	sRequest	+= SG_T("&VERSION=1.1.1");
	sRequest	+= SG_T("&REQUEST=GetCapabilities");

	//-------------------------------------------------
	wxInputStream	*pStream;

	if( (pStream = m_Server.GetInputStream(sRequest.c_str())) != NULL )
	{
		wxXmlDocument	Capabilities;

		if( Capabilities.Load(*pStream) )
		{
			bResult	= _Get_Capabilities(Capabilities.GetRoot());

			Capabilities.Save(CSG_String::Format(SG_T("e:\\%s.xml"), m_Title.c_str()).c_str());
		}

		delete(pStream);
	}


	return( bResult );
}



bool CWMS_Capabilities::_Get_Capabilities(wxXmlNode *pRoot)
{
	wxXmlNode	*pNode, *pChild;

	//-----------------------------------------------------
	// 1. Service

	if( (pNode = CWMS_XmlHandlers::_Get_Child(pRoot, SG_T("Service"))) == NULL )
	{
		return( false );
	}

	//CWMS_XmlHandlers::_Get_Node_PropVal (pRoot, m_Version, SG_T("version")); TODO
	m_Version = SG_T("1.1.1");
	if (!m_Version.Cmp( SG_T("1.3.0")))	m_ProjTag = SG_T("CRS");//TODO pada kdyz se nerovna s getcap
	else					m_ProjTag = SG_T("SRS");



	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Name		, SG_T("Name"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Title		, SG_T("Title"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Abstract		, SG_T("Abstract"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Fees		, SG_T("Fees"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Access		, SG_T("AccessConstraints"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MaxLayers		, SG_T("LayerLimit"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MaxWidth		, SG_T("MaxWidth"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MaxHeight		, SG_T("MaxHeight"));
	CWMS_XmlHandlers::_Get_Child_PropVal(pNode, m_Online		, SG_T("OnlineResource"), SG_T("xlink:href"));

	if( (pChild = CWMS_XmlHandlers::_Get_Child(pNode, SG_T("KeywordList"))) != NULL )
	{
		m_Keywords = CWMS_XmlHandlers::_Get_Children_Content(pNode,"keyword");
	}
	if( (pChild = CWMS_XmlHandlers::_Get_Child(pNode, SG_T("ContactInformation"))) != NULL )//TODO
	{
	}


	//-----------------------------------------------------
	// 2. Capabilities

	if( (pNode = CWMS_XmlHandlers::_Get_Child(pRoot, SG_T("Capability"))) == NULL )
	{
		return( false );
	}

	//-----------------------------------------------------
	// 2.a) Request

	if( (pChild = CWMS_XmlHandlers::_Get_Child(CWMS_XmlHandlers::_Get_Child(CWMS_XmlHandlers::_Get_Child(pNode, SG_T("Request")), SG_T("GetMap")), SG_T("Format"))) != NULL )//TODO 1.0
	{
		if( !m_Version.Cmp(SG_T("1.0.0")) )//TODO
		{
			pChild	= pChild->GetChildren();

			while( pChild )
			{
				m_Formats	+= pChild->GetName().c_str();
				m_Formats	+= SG_T("|");

				pChild	= pChild->GetNext();
			}
		}
		else
		{
			do
			{
				if( !pChild->GetName().CmpNoCase(SG_T("Format")) )
				{
					m_Formats	+= pChild->GetNodeContent().c_str();
					m_Formats	+= SG_T("|");
				}
			}
			while( (pChild = pChild->GetNext()) != NULL );
		}
	}

	//-----------------------------------------------------
	// 2.b) Exception, Vendor Specific Capabilities, User Defined Symbolization, ...


	//-----------------------------------------------------
	// 2.c) Layers


	if( (pNode = CWMS_XmlHandlers::_Get_Child(pNode, SG_T("Layer"))) == NULL )
	{
		// Message_Add(_TL("No layer tag was found in capabilities response"));
		return false;
	}
	int id = 0;

	_Load_Layers(pNode, id);

	//-----------------------------------------------------
	return( true );

}




bool  CWMS_Capabilities:: _Load_Layers	(class wxXmlNode *pNode, int & layer_id, CWMS_Layer *parrent_layer, int nDepth)

{
	wxXmlNode   *pChild;

	CWMS_Layer	*pLayer	= new CWMS_Layer;
	pLayer->Create(pNode, parrent_layer, layer_id ,m_ProjTag );

	if(nDepth == 0)
	{
		m_RootLayer = pLayer;

	}
	nDepth++;

	pChild = pNode->GetChildren();
	do
	{
		if( !pChild->GetName().CmpNoCase( SG_T("Layer") ))
		{

			_Load_Layers(pChild, ++layer_id,pLayer, nDepth);
		}
	}
	while( (pChild = pChild->GetNext()) != NULL );

	return true;
}


CSG_String CWMS_Capabilities::Get_Summary(void)//TODO print it
{
	CSG_String	s;

	if( m_Name.Length() > 0 )
	{
		s	+= SG_T("\n[Name] ")			+ m_Name		+ SG_T("\n");
	}

	if( m_Title.Length() > 0 )
	{
		s	+= SG_T("\n[Title] ")			+ m_Title		+ SG_T("\n");
	}

	if( m_Abstract.Length() > 0 )
	{
		s	+= SG_T("\n[Abstract] ")		+ m_Abstract	+ SG_T("\n");
	}

	if( m_Fees.Length() > 0 )
	{
		s	+= SG_T("\n[Fees] ")			+ m_Fees		+ SG_T("\n");
	}

	if( m_Online.Length() > 0 )
	{
		s	+= SG_T("\n[Online Resource] ")	+ m_Online		+ SG_T("\n");
	}

	if( m_Keywords.Get_Count() > 0 )
	{
		s	+= SG_T("\n[Keywords] ");

		for(int i=0; i<m_Keywords.Get_Count(); i++)
		{
			if( i > 0 )	s	+= SG_T(", ");

			s	+= m_Keywords[i];
		}

		s	+= SG_T("\n");
	}

	if( m_MaxLayers > 0 )
	{
		s	+= CSG_String::Format(SG_T("\n[Max. Layers] %d\n"), m_MaxLayers);
	}

	if( m_MaxWidth > 0 )
	{
		s	+= CSG_String::Format(SG_T("\n[Max. Width] %d\n"), m_MaxWidth);
	}

	if( m_MaxHeight > 0 )
	{
		s	+= CSG_String::Format(SG_T("\n[Max. Height] %d\n"), m_MaxHeight);
	}

	if( m_Contact.Length() > 0 )
	{
		s	+= SG_T("\n[Contact] ")			+ m_Contact		+ SG_T("\n");
	}

	if( m_Access.Length() > 0 )
	{
		s	+= SG_T("\n[Access] ")			+ m_Access		+ SG_T("\n");
	}

	return( s );
}



void CWMS_Capabilities::Get_Layers(CWMS_Layer * layer, std::vector<CWMS_Layer*> & layers)
{
	if(layers.empty())	layers.push_back(layer);

	for(int i = 0; i < layer->m_Layers.size(); i++)
	{
		layers.push_back( layer->m_Layers[i] );
		Get_Layers(layer->m_Layers[i], layers);
	}

}

CSG_Strings CWMS_Capabilities::Proj_Intersect(std::vector<CWMS_Layer*> & layers  )
{
	CSG_Strings intersProjs;
	CSG_Strings intersProjsTemp;

	if(layers.size() > 0)
	{
		for(int i_proj = 0; i_proj <  layers[0]->Get_Projections().size(); i_proj++ )
		{
			intersProjs.Add(layers[0]->Get_Projections()[i_proj].m_Projection);
		}
	}

	for(int i_Num = 1; i_Num < layers.size(); i_Num++)
	{

		intersProjsTemp.Clear();
		std::vector<class CWMS_Projection> projections = layers[i_Num]->Get_Projections();

		for(int i_proj = 0; i_proj < projections.size(); i_proj++ )
		{
			bool isInside =false;
			for(int i_intersecPproj1 = 0; i_intersecPproj1 < intersProjs.Get_Count(); i_intersecPproj1++ )
			{
				if( !projections[i_proj].m_Projection.Cmp(intersProjs[i_intersecPproj1]) )
				{
					isInside = true;
					break;
				}
			}
			if(isInside) intersProjsTemp.Add( projections[i_proj].m_Projection );
		}
		intersProjs = intersProjsTemp;
	}
	return intersProjs;
}

