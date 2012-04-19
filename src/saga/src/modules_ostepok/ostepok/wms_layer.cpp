#include <wx/xml/xml.h>
#include <wx/image.h>

#include "wms_import.h"


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

bool	CWMS_Layer::Create ( wxXmlNode * pNode, CWMS_Layer * pParrentLayer, const int layer_id, CSG_String m_ProjTag)
{

	if(pParrentLayer == NULL)
	{
		m_pParrentLayer = NULL;
	}
	else
	{
		pParrentLayer->m_Layers.push_back(this);
		m_pParrentLayer = pParrentLayer;//TODO setParrentLayer
		Assign_Projs(pParrentLayer->Get_Projections());
		Assign_Styles(pParrentLayer->Get_Styles());
		m_MaxScaleD = pParrentLayer->m_MaxScaleD;
		m_MinScaleD = pParrentLayer->m_MinScaleD;
	}



	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Name, SG_T("Name"));// TODO merge
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Title, SG_T("Title"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_Abstract, SG_T("Abstract"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MaxScaleD, SG_T("MaxScaleDenomminator"));
	CWMS_XmlHandlers::_Get_Child_Content(pNode, m_MinScaleD, SG_T("MinScaleDenomminator"));
	m_id = layer_id;

	CSG_String propVal;
	CWMS_XmlHandlers::_Get_Node_PropVal(pNode, propVal, "queryalbe");//TODO cyklus
	if(propVal == SG_T("1")) m_Querryable = false;
	propVal.Clear();

	CWMS_XmlHandlers::_Get_Node_PropVal(pNode, propVal, "opaque");//TODO cyklus
	if(propVal == SG_T("1")) m_Opaque = false;
	propVal.Clear();

	CWMS_XmlHandlers::_Get_Node_PropVal(pNode, propVal, "noSubsets");//TODO cyklus
	if(propVal == SG_T("1")) m_Querryable = false;
	propVal.Clear();

	//CWMS_XmlHandlers::_Get_Node_PropVal(pNode, propVal, "cascaded");//TODO cyklus
	//if(propVal.Length() == 0) m_Cascaded = CSG_String::Format( SG_T("%d", propVal));
	//propVal.Clear();


	wxXmlNode   *pChild;
	if( (pChild = CWMS_XmlHandlers::_Get_Child(pNode, SG_T("KeywordList"))) != NULL )
	{
		m_KeywordList = CWMS_XmlHandlers::_Get_Children_Content(pChild,"keyword");
	}

	m_Dimensions = CWMS_XmlHandlers::_Get_Children_Content(pNode,"dimension");


	pChild = pNode->GetChildren();
	do
	{
		if( !pChild->GetName().CmpNoCase( SG_T("Style") ) )
		{
		    Add_Style(pChild);
		}

		if( !pChild->GetName().CmpNoCase( m_ProjTag ) )
		{
		    Add_Proj(pChild);
		}

	}
	while( (pChild = pChild->GetNext()) != NULL );

	pChild = pNode->GetChildren();
	do
	{
		if( !pChild->GetName().CmpNoCase( SG_T("BoundingBox") ) )
		{
		    Add_GeoBBox(pChild, m_ProjTag );
		}

	}
	while( (pChild = pChild->GetNext()) != NULL );

	return true;

}



bool CWMS_Layer::Add_Style ( wxXmlNode * pStyleNode)
{

	bool bStyleExists = false;
	CSG_String insertedStyleName;

	for(int i_style = 0; i_style < m_Styles.size(); i_style++)
	{
		CWMS_XmlHandlers::_Get_Child_Content(pStyleNode, insertedStyleName, SG_T("Name"));
		if( m_Styles[i_style].m_Name.CmpNoCase(insertedStyleName) == 0)
		{
			bStyleExists = true;
			break;
		}
	}

	if(!bStyleExists)
	{
		CWMS_Style style;
		CWMS_XmlHandlers::_Get_Child_Content(pStyleNode, style.m_Name, SG_T("Name"));
		CWMS_XmlHandlers::_Get_Child_Content(pStyleNode, style.m_Title, SG_T("Title"));
		m_Styles.push_back(style);
	}

	return true;
}




bool CWMS_Layer::Add_Proj ( wxXmlNode * pProjNode)
{

	bool bProjExists = false;
	for(int i_proj = 0; i_proj < m_Projections.size(); i_proj++)
	{
		if( m_Projections[i_proj].m_Projection.CmpNoCase(pProjNode->GetNodeContent().c_str()) == 0)//TODO nocase
		{
			bProjExists = true;
			break;
		}
	}

	if(!bProjExists)
	{
		CWMS_Projection projection;
		projection.m_Projection =  pProjNode->GetNodeContent().c_str();
		m_Projections.push_back(projection);
	}

	return true;
}

bool CWMS_Layer::Add_GeoBBox ( wxXmlNode * pBboxNode, CSG_String projTag)//todo
{
	for(int i_proj = 0; i_proj < m_Projections.size(); i_proj++)
	{
		CSG_String	s;
		CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, projTag);

		if( !m_Projections[i_proj].m_Projection.CmpNoCase(s))
		{

			if(			!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, "minx") && s.asDouble(m_Projections[i_proj].m_GeoBBox.xMin))//TODO
					||	!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, "miny") && s.asDouble(m_Projections[i_proj].m_GeoBBox.yMin))
					||	!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, "maxx") && s.asDouble(m_Projections[i_proj].m_GeoBBox.xMax))
					||	!(CWMS_XmlHandlers::_Get_Node_PropVal(pBboxNode, s, "maxy") && s.asDouble(m_Projections[i_proj].m_GeoBBox.yMax)) )
			{
				m_Projections[i_proj].m_GeoBBox.xMin	= m_Projections[i_proj].m_GeoBBox.yMin	= m_Projections[i_proj].m_GeoBBox.xMax	= m_Projections[i_proj].m_GeoBBox.yMax	= 0.0;


			}

		}

	}

	return true;

}

void CWMS_Layer::_Delete_Child_Layers(void)
{
    for(int i=0; i < m_Layers.size(); i++)
    {
	    delete(m_Layers[i]);
    }


}
