#include "wms_import.h"
#include <wx/xml/xml.h>


CWMS_Layer::CWMS_Layer(void)
{
	m_pParrentLayer = NULL;
	m_Opaque	= false;
	m_Querryable	= false;
	no_Subsets	= false;
	m_Cascaded	= 0;
	m_fixedWidth	= 0;
	m_fixedHeight	= 0;

}

CWMS_Layer::~CWMS_Layer(void)
{
	_Delete_Child_Layers();
}


void CWMS_Layer::_Delete_Child_Layers(void)
{
// removes layers recursively
	for(int i=0; i < m_LayerChildren.size(); i++)
	{
		delete(m_LayerChildren[i]);
	}
}

void	CWMS_Layer::Create ( wxXmlNode * pLayerNode, CWMS_Layer * pParrentLayer, const int layerId, wxString m_ProjTag)
{
// parses layer attributes
	if(pParrentLayer == NULL)
	{
		m_pParrentLayer = NULL;
	}

	else
	{
		// connecting the layer with parrent layer and oposite
		m_pParrentLayer = pParrentLayer;
		pParrentLayer->m_LayerChildren.push_back(this);

		//attributes which are inherited from parrent layer
		Assign_Projs(pParrentLayer->Get_Projections());
		Assign_Styles(pParrentLayer->Get_Styles());
		m_MaxScaleD = pParrentLayer->m_MaxScaleD;
		m_MinScaleD = pParrentLayer->m_MinScaleD;
	}

	m_id = layerId;

	// getting values from GetCapabilities response
	CWMS_XmlHandlers::_Get_Child_Content(pLayerNode, m_Name, wxT("Name"));
	CWMS_XmlHandlers::_Get_Child_Content(pLayerNode, m_Title, wxT("Title"));
	CWMS_XmlHandlers::_Get_Child_Content(pLayerNode, m_Abstract, wxT("Abstract"));
	CWMS_XmlHandlers::_Get_Child_Content(pLayerNode, m_MaxScaleD, wxT("MaxScaleDenomminator"));
	CWMS_XmlHandlers::_Get_Child_Content(pLayerNode, m_MinScaleD, wxT("MinScaleDenomminator"));

	wxString propVal;
	CWMS_XmlHandlers::_Get_Node_PropVal(pLayerNode, propVal, wxT("queryalbe"));
	if(propVal == wxT("1")) m_Querryable = false;
	propVal.Clear();

	CWMS_XmlHandlers::_Get_Node_PropVal(pLayerNode, propVal, wxT("opaque"));
	if(propVal == wxT("1")) m_Opaque = false;
	propVal.Clear();

	CWMS_XmlHandlers::_Get_Node_PropVal(pLayerNode, propVal, wxT("noSubsets"));
	if(propVal == wxT("1")) m_Querryable = false;
	propVal.Clear();

	CWMS_XmlHandlers::_Get_Node_PropVal(pLayerNode, propVal, wxT("cascaded"));
	if(propVal.Length() == 0)   propVal.ToLong((long*)&m_Cascaded);
	propVal.Clear();

	CWMS_XmlHandlers::_Get_Node_PropVal(pLayerNode, propVal, wxT("fixedWidth"));
	if(propVal.Length() == 0)   propVal.ToLong((long*)&m_fixedWidth);
	propVal.Clear();

	CWMS_XmlHandlers::_Get_Node_PropVal(pLayerNode, propVal, wxT("fixedHeiht"));
	if(propVal.Length() == 0)   propVal.ToLong((long*)&m_fixedHeight);
	propVal.Clear();

	wxXmlNode   *pChild;
	if( (pChild = CWMS_XmlHandlers::_Get_Child(pLayerNode,wxT("KeywordList"))) != NULL )
	{
		m_KeywordList = CWMS_XmlHandlers::_Get_Children_Content(pChild, wxT("keyword"));
	}

	m_Dimensions = CWMS_XmlHandlers::_Get_Children_Content(pLayerNode,wxT("dimension"));

	pChild = pLayerNode->GetChildren();
	do
	{
		if( !pChild->GetName().CmpNoCase( wxT("Style") ) )
		{
		    Add_Style(pChild);
		}

		if( !pChild->GetName().CmpNoCase( m_ProjTag ) )
		{
		    Add_Proj(pChild);
		}

	}
	while( (pChild = pChild->GetNext()) != NULL );

	pChild = pLayerNode->GetChildren();
	do
	{
		if( !pChild->GetName().CmpNoCase( wxT("BoundingBox") ) )
		{
		    Add_GeoBBox(pChild, m_ProjTag );
		}

	}
	while( (pChild = pChild->GetNext()) != NULL );
}


void CWMS_Layer::Add_Style ( wxXmlNode * pStyleNode)
{
//checks if the style is not already inherited
	bool bStyleExists = false;
	wxString insertedStyleName;

	for(int i_style = 0; i_style < m_Styles.size(); i_style++)
	{
		CWMS_XmlHandlers::_Get_Child_Content(pStyleNode, insertedStyleName, wxT("Name"));
		if( m_Styles[i_style].m_Name.CmpNoCase(insertedStyleName) == 0)
		{
			bStyleExists = true;
			break;
		}
	}

	if(!bStyleExists)
	{
		CWMS_Style style;
		CWMS_XmlHandlers::_Get_Child_Content(pStyleNode, style.m_Name, wxT("Name"));
		CWMS_XmlHandlers::_Get_Child_Content(pStyleNode, style.m_Title, wxT("Title"));
		m_Styles.push_back(style);
	}
}


void CWMS_Layer::Add_Proj ( wxXmlNode * pProjNode)
{
//checks if the projection is not already inherited
	bool bProjExists = false;
	for(int i_proj = 0; i_proj < m_Projections.size(); i_proj++)
	{
		if( m_Projections[i_proj].m_Projection.CmpNoCase(pProjNode->GetNodeContent()) == 0)
		{
			bProjExists = true;
			break;
		}
	}

	if(!bProjExists)
	{
		CWMS_Projection projection;
		projection.m_Projection =  pProjNode->GetNodeContent();
		m_Projections.push_back(projection);
	}
}


void CWMS_Layer::Add_GeoBBox ( wxXmlNode * pBboxNode, wxString projTag)
{
//appends bbox to corresponding projection
	for(int i_proj = 0; i_proj < m_Projections.size(); i_proj++)
	{
		wxString	s;
		CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, projTag);

		if( !m_Projections[i_proj].m_Projection.CmpNoCase(s))
		{
			if(			!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, wxT("minx")) && s.ToDouble(&m_Projections[i_proj].m_GeoBBox.xMin))
					||	!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, wxT("miny")) && s.ToDouble(&m_Projections[i_proj].m_GeoBBox.yMin))
					||	!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, wxT("maxx")) && s.ToDouble(&m_Projections[i_proj].m_GeoBBox.xMax))
					||	!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, wxT("maxy")) && s.ToDouble(&m_Projections[i_proj].m_GeoBBox.yMax)) )
			{
				m_Projections[i_proj].m_GeoBBox.xMin	= m_Projections[i_proj].m_GeoBBox.yMin	= m_Projections[i_proj].m_GeoBBox.xMax	= m_Projections[i_proj].m_GeoBBox.yMax	= 0.0;
			}
		}
	}
}


