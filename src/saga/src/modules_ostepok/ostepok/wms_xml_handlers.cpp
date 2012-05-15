#include "wms_import.h"

std::vector<wxString>  CWMS_XmlHandlers::Get_Children_Content(wxXmlNode * pNode, const wxString & children_name)
{
	std::vector<wxString> childrenContent;
	std::vector<wxXmlNode* > pChildren = Get_Children(pNode);
	wxXmlNode * pChild;

	for(int i_Child = 0; i_Child < childrenContent.size(); i_Child++)
	{
		pChild = pChildren[i_Child];
		if( pChild->GetName().CmpNoCase(children_name) )
		{
			childrenContent.push_back(pChild->GetNodeContent());
		}
	}

	return childrenContent;
}


std::vector<wxXmlNode *>  CWMS_XmlHandlers::Get_Children(wxXmlNode * pNode)
{
	std::vector<wxXmlNode *> children;
	wxXmlNode * pItem = pNode->GetChildren();

	while(pItem)
	{
		children.push_back(pItem);
		pItem = pItem->GetNext();
	}

	return children;
}


std::vector<wxXmlNode *>  CWMS_XmlHandlers::Get_Children(wxXmlNode * pNode, const wxString &Name)
{
	std::vector<wxXmlNode *> children;

	if( pNode && (pNode = pNode->GetChildren()) != NULL )
	{
		do
		{
			if( !pNode->GetName().CmpNoCase(Name) )
			{
				children.push_back(pNode);
			}
		}
		while( (pNode = pNode->GetNext()) != NULL );
	}

	return children;
}

wxXmlNode * CWMS_XmlHandlers::Get_Child(wxXmlNode *pNode, const wxString &Name)
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

bool CWMS_XmlHandlers::Get_Child_Content(wxXmlNode *pNode, wxString &Value, const wxString &Name)
{
	if( (pNode = Get_Child(pNode, Name)) != NULL )
	{
		Value	= pNode->GetNodeContent();

		return( true );
	}

	return( false );
}

bool CWMS_XmlHandlers::Get_Child_Content(wxXmlNode *pNode, int &Value, const wxString &Name)
{
	long	lValue;

	if( (pNode = Get_Child(pNode, Name)) != NULL && pNode->GetNodeContent().ToLong(&lValue) )
	{
		Value	= lValue;

		return( true );
	}

	return( false );
}

bool CWMS_XmlHandlers::Get_Child_Content(wxXmlNode *pNode, double &Value, const wxString &Name)
{
	double	dValue;

	if( (pNode = Get_Child(pNode, Name)) != NULL && pNode->GetNodeContent().ToDouble(&dValue) )
	{
		Value	= dValue;

		return( true );
	}

	return( false );
}

bool CWMS_XmlHandlers::Get_Child_PropVal(wxXmlNode *pNode, wxString &Value, const wxString &Name, const wxString &Property)
{
	return( (pNode = Get_Child(pNode, Name)) != NULL && pNode->GetPropVal(Property, &Value) );
}
