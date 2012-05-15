#include "wms_import.h"


CWMS_Layer::CWMS_Layer(void)
{
	m_pParrentLayer = NULL;
}

CWMS_Layer::~CWMS_Layer(void)
{
	_Delete_Child_Layers();
}


void CWMS_Layer::_Delete_Child_Layers(void)
{
//removes layers recursively
	for(int i=0; i < m_ChildLayers.size(); i++)
	{
		delete(m_ChildLayers[i]);
	}
}

void	CWMS_Layer::Create (wxXmlNode * pLayerNode, CWMS_Layer * pParrentLayer, const int layerId, CWMS_XmlHandlers * pXmlH, wxString projTag)
{

	m_id = layerId;
	m_pParrentLayer = pParrentLayer;
	m_pXmlH = pXmlH;
	m_ProjTag = projTag;
	m_pLayerNode = pLayerNode;

	// Solving inheritence in GetCapabilities response according to WMS standard //


	// Simple inheritance of tags. If not present in <Layer>, inherited from parent layer if is present. Except <Attribution>, which is always added.
	wxArrayString inhElms;
	inhElms.Add(wxString(wxT("EX_GeographicBoundingBox")));
	inhElms.Add(wxString(wxT("Attribution")));
	inhElms.Add(wxString(wxT("MinScaleDenominator")));
	inhElms.Add(wxString(wxT("MaxScaleDenominator")));
	inhElms.Add(wxString(wxT("AuthorityURL")));

	std::vector<wxXmlNode *> nodes;
	std::vector<wxXmlNode *> parent_nodes;
	for (int i_Elm = 0; i_Elm < inhElms.Count(); i_Elm++)
	{
		nodes = m_pXmlH->Get_Children(m_pLayerNode, inhElms[i_Elm]);
		if((nodes.size() == 0 || !inhElms[i_Elm].Cmp(wxT("Attribution"))) && m_pParrentLayer)
		{
			parent_nodes =m_pXmlH->Get_Children(m_pParrentLayer->m_pLayerNode, inhElms[i_Elm]);
			nodes.insert(nodes.end(), parent_nodes.begin(), parent_nodes.end());
		}
		m_LayerElms.insert(std::make_pair(inhElms[i_Elm], nodes));
	}

	//inheritance of attributes of <Layer> tag. If attribute is not present in <Layer>, inherited from parent layer if is present.
	std::vector<wxString> inhAttribs;
	inhAttribs.push_back(wxString(wxT("queryable")));
	inhAttribs.push_back(wxString(wxT("cascaded")));
	inhAttribs.push_back(wxString(wxT("opaque")));
	inhAttribs.push_back(wxString(wxT("noSubsets")));
	inhAttribs.push_back(wxString(wxT("fixedWidth")));
	inhAttribs.push_back(wxString(wxT("fixedHeight")));

	wxString attrValue;
	for (int i_Attr = 0; i_Attr < inhAttribs.size(); i_Attr++)
	{
		if(!m_pParrentLayer) break;
		if(!m_pLayerNode->HasProp(inhAttribs[i_Attr]) &&
		    m_pParrentLayer->m_pLayerNode->GetPropVal(inhAttribs[i_Attr], &attrValue))
		{
			m_pLayerNode->AddAttribute(inhAttribs[i_Attr], attrValue);
		}
	}

	// tags which are checked if already tag with checked parameter (element_content, it`s attribute  or child_element_content)
	// is present in layer. If the tag with parameter is not in the layer it is inherited from parent layer

	_inhNotSameElement(m_ProjTag, wxT("element_content"), inhElms);
	_inhNotSameElement(wxT("BoundingBox"), wxT("attribute"), inhElms, m_ProjTag);
	_inhNotSameElement(wxT("Style"), wxT("child_element_content"), inhElms, wxT("Name"));
	_inhNotSameElement(wxT("Dimension"), wxT("attribute"), inhElms, wxT("name"));

	// in order to map m_LayerElements contain all nodes of Layer tag, missing not inherited elements are added

	wxString elem;
	std::vector<wxXmlNode *> newVect;
	nodes = m_pXmlH->Get_Children(m_pLayerNode);

	for (int i_NodeElem = 0; i_NodeElem < nodes.size(); i_NodeElem++)
	{
		elem = nodes[i_NodeElem]->GetName();
		if(inhElms.Index(elem) != wxNOT_FOUND) continue;

		std::map<wxString, std::vector<wxXmlNode *> >::iterator it = m_LayerElms.find(elem);
		if (it != m_LayerElms.end())
		{
			it->second.push_back(nodes[i_NodeElem]);
		}
		else
		{
			newVect.clear();
			newVect.push_back(nodes[i_NodeElem]);
			m_LayerElms.insert(std::make_pair(elem, newVect));
		}
	}
}

void	CWMS_Layer::_inhNotSameElement (const wxString & elmName, const wxString & cmpType, wxArrayString & inhElems, const wxString & addArg)
{

	std::vector<wxXmlNode *> nodes = m_pXmlH->Get_Children(m_pLayerNode, elmName);

	std::vector<wxXmlNode *> parentNodes;
	if(m_pParrentLayer)
	{
		if( m_pParrentLayer->m_LayerElms.find(elmName) !=  m_pParrentLayer->m_LayerElms.end())
			parentNodes = m_pParrentLayer->m_LayerElms[elmName];
	}


	wxXmlNode * pParentNode;
	wxXmlNode * pNode;

	wxString parentCmpText, cmpText;
	for(int i_ParentNode = 0; i_ParentNode < parentNodes.size(); i_ParentNode++)
	{
		pParentNode = parentNodes[i_ParentNode];
		parentCmpText.Empty();
		if(!cmpType.Cmp(wxT("attribute")))
			 pParentNode->GetPropVal(addArg, &parentCmpText);

		else if(!cmpType.Cmp(wxT("element_content")))
			parentCmpText = pParentNode->GetNodeContent();

		else if(!cmpType.Cmp(wxT("child_element_content")))
			 m_pXmlH->Get_Child_PropVal(pParentNode, parentCmpText, elmName, addArg);

		if (parentCmpText.IsEmpty())
			continue;

		bool isThere  = false;

		for(int i_Node = 0; i_Node < nodes.size(); i_Node++)
		{
			pNode = nodes[i_Node];
			cmpText.Empty();
			if(!cmpType.Cmp(wxT("attribute")))
				    pNode->GetPropVal(addArg, &cmpText);

			else if(!cmpType.Cmp(wxT("element_content")))
				cmpText = pNode->GetNodeContent();

			else if(!cmpType.Cmp(wxT("child_element_content")))
				m_pXmlH->Get_Child_PropVal(pNode, cmpText, elmName, addArg);

			if(!cmpText.CmpNoCase(parentCmpText))
			{
				isThere = true;
				break;
			}
		}
		if (!isThere)
		{
			nodes.push_back(pParentNode);
		}
	}

	m_LayerElms.insert(std::make_pair(elmName, nodes));
	inhElems.Add(elmName);
}

std::vector<wxXmlNode *> CWMS_Layer::getElms (const wxString & elementsName)
{
	std::vector<wxXmlNode *> elms;

	if (m_LayerElms.find(elementsName) != m_LayerElms.end())
		    elms = m_LayerElms.find(elementsName)->second;

	return elms;
}
