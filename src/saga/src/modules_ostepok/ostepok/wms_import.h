
///////////////////////////////////////////////////////////
//                                                       //
//                         SAGA                          //
//                                                       //
//      System for Automated Geoscientific Analyses      //
//                                                       //
//                    Module Library:                    //
//                    lib_webservices                    //
//                                                       //
//-------------------------------------------------------//
//                                                       //
//                      wms_import.h                     //
//                                                       //
//                 Copyright (C) 2012 by                 //
//			Stepan Turek                     //
//                 based on Olaf Conrad's code           //
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
//    e-mail:     stepan.turek@seznam.cz                 //
//                                                       //
//                                                       //
///////////////////////////////////////////////////////////

//---------------------------------------------------------


#ifndef HEADER_INCLUDED__WMS_Import_H
#define HEADER_INCLUDED__WMS_Import_H

#include "MLB_Interface.h"

#include <map>
#include <vector>
#include <stdexcept>

#include <wx/string.h>
#include <wx/xml/xml.h>
#include <wx/protocol/http.h>


class CWMS_Layer;
class CWMS_XmlHandlers;
class CWMS_Base;
class CWMS_Gdal_drv;
class CWMS_Projection;

 //! Exception class used for reporting of errors in CWMS_WMS_Base class and its members
 class CWMS_Exception : public std::runtime_error
 {
 public:

     CWMS_Exception(const wxString& msg = wxT("Exception not specified")) :  runtime_error( std::string(msg.mb_str())) {}
 };



 //! Class of the module
class CWMS_Import : public CSG_Module
{
public:

	/*!
	Constructor creates the first dialog
	*/
	CWMS_Import(void);

	virtual ~CWMS_Import();

protected:


	CWMS_Base *				m_WMS;/*!< holds reference to CWMS_WMS_Base class which is used for creating of WMS request and communication with WMS server*/

	CSG_Parameter *				m_pOutputMap;

	//! Main method of the module, which is called when modul is executed
	/*!
	The method creates instance of CWMS_WMS_Base (m_WMS) and then fetches capabilities file with m_Capapilities member of m_WMS, shows two dialogs _Create_Layers_Dialog
	and _Create_Settings_Dialog and then get chosen information from the dialogs in order to be able to fetch data from WMS server with Get_Map method of m_WMS.
	The data are imported into SAGA with _ImportMap method.
	*/
	virtual bool				On_Execute		(void);

	//! Creates data for dialog with layer tree.
	/*!
	\param layers vector with layers for the tree.
	\param parLayers paremeter object, where the tree will be stored
	*/
	bool					_Create_Layers_Dialog	(const std::vector<CWMS_Layer*> & layers, CSG_Parameters & parLayers);

	//! Creates data for dialog with other WMS request settings.
	/*!
	\param layers vector with layers for the tree.
	\param parLayers paremeter object, where will be stored data for callbackfunction _Settings_Dial_Callback and return parametrs to show in dialog.
	\returns parameters which will be show in dialog
	*/
	CSG_Parameters *			_Create_Settings_Dialog	(const std::vector<CWMS_Layer*> & selectedLayers, CSG_Parameters & parSettings);

	//! Imports GRID to SAGA from file image.
	/*!
	\param tempMapPath path to image file for import
	\returns true if import was successful
	*/
	bool					_Import_Map		(const wxString & tempMapPath);

	//! Finds all joint projections for layers.
	/*!
	\returns vector of CWMS_Projection objects. In CWMS_Projection is stored projection and corresponding bounding box.
	*/
	std::vector<CWMS_Projection>		_Proj_Intersect		(const std::vector<CWMS_Layer*> & layers  );

	//!	   Changes boundix box values according to chosen projection and layers in layer order selects.
	static int				_Settings_Dial_Callback	(CSG_Parameter * pParameter, int Flags);

	/*!
	\returns layer title, if it is empty then returns layer name and if name is also empty returns layer id
	*/
	CSG_String				_Get_Layers_Existing_Name(CWMS_Layer * pLayer);
};


//! Unites helper function for handling wxXmlNodes
class CWMS_XmlHandlers
{
public:
	/*!
	\return all children of pNode in vector.
	*/
	std::vector<wxXmlNode *>	Get_Children		(wxXmlNode * pNode);

	/*!
	\return children of pNode in vector which have tag name same as Name.
	*/
	std::vector<wxXmlNode *>	Get_Children		(wxXmlNode * pNode, const wxString &Name);

	/*!
	\return first found child of pNode which has tag name same as Name.
	*/
	wxXmlNode *			Get_Child		(wxXmlNode *pNode, const wxString &Name);

	/*!
	\param Value will contain content of first found child of pNode  which has tag name same as Name and will return true.
	\return  false if child is not found and \param Value will not be changed.
	*/
	bool				Get_Child_Content	(wxXmlNode *pNode, wxString & Value, const wxString &Name);

	 /*!
	Value will contain int value in content of first found child of pNode  which has tag name same as Name and will return true.
	\return  false if child is not found or it's content cannot be converted to int and Value is not changed in this case.
	*/
	bool				Get_Child_Content	(wxXmlNode *pNode, int & Value, const wxString &Name);

	/*!
	Value will contain int value in content of first found child of pNode  which has tag name same as Name and will return true.
	\return  false if child is not found or it's content cannot be converted to double and Value and Value is not changed in this case.
	*/
	bool				Get_Child_Content	(wxXmlNode *pNode, double & Value, const wxString &Name);

	/*!
	Value will contain attribute Property value of first found child of pNode  which has tag name same as Name and will return true.
	\return false if child or child's attribute are not found and Value is not changed in this case.
	*/
	bool				Get_Child_PropVal	(wxXmlNode *pNode, wxString & Value, const wxString &Name, const wxString &Property);

	/*!
	\return contents of children of pNode in vector which have tag name same as children_name.
	*/
	std::vector<wxString>		Get_Children_Content	(wxXmlNode *pNode, const wxString &children_name);
};




//! Downloades Capabilities file from WMS server and parses it into own data structure
class CWMS_Capabilities
{
public:

	CWMS_Capabilities(void);

	//! Calls _Reset method
	/*!
	   \sa _Reset()
	*/
	~CWMS_Capabilities(void);

	wxString			m_Version;/*!< holds value of WMS version*/

	wxString			m_ProjTag; /*!< holds value CRS (v. => 1.3.0) or SRS (v. <= 1.1.0.), depends on  WMS version got from GetCap file*/

	CWMS_XmlHandlers                m_XmlH; /*!< CWMS_XmlHandlers object, which is used also by tree of CWMS_Layer (in layers there is stored pointer to this object) */

	wxXmlNode *			m_pCapNode; /*!< Contains whole <Capability> node*/

	wxXmlNode *			m_pServiceNode; /*!< Contains whole <Service> node*/

	CWMS_Layer *			m_RootLayer; /*!<  Contains whole <Layer> tree*/

	//! Sends GetCapabilities request to WMS server, fetch response, and calls \param _InitializeCapabilities method.
	/*!
	    \param serverUrl URL adress of WM_Create_Settings_DialogS server
	    \param userName  username, if it is required by server
	    \param password  password, if it is required by server
	*/
	void				Create			(const wxString & serverUrl, const wxString & userName, const wxString & password);

	//!  \param layers will containt all layers in tree. \param layer must be left empty, it is used for recursive calling of the method
	void				Get_Layers		(std::vector<CWMS_Layer* > & layers, CWMS_Layer * layer = NULL);

protected:

	wxXmlDocument			m_XmlTree; /*!< Contains whole parsed GetCapabilities XML file*/

	//! Parses WMS XML file pRoot then  calls _InitializeLayerTree function for parsing of <Layer> tag
	void				_InitializeCapabilities	(class wxXmlNode *pRoot);

	//! Removes CWMS_Layer tree recursively with delete statement
	void				_Reset			(void);

	//! Recursive function, which initializes all layers in GetCapabilities file
	/*!
	    \param pNode node with <Layer> tag
	    \param layer_id  id of initialized layer
	    \param parrent_layer  For root <Layer> is null, because it does not have any parent
	    \param nDepth level of depth in the tree
	*/
	CWMS_Layer *                    _InitializeLayerTree	(class wxXmlNode * pLayerNode, int & layerId  ,CWMS_Layer * pParrentLayer = NULL, int nDepth = 0);
};


//! Abstract class, which cumunicates with WMS server (supports GetCapabilities and GetMap requests)
/*!
  Information from GetCapabilities response are stored in m_Capabilities object.
  Class members store chosen values of parameters for GetMap request.
*/
class CWMS_Base
{
public:

	CWMS_Base(void);

	virtual ~CWMS_Base(void);

	CWMS_Capabilities			m_Capabilities;/*!< holds value of WMS version*/

	//Values for GetMap Request,  TODO violated principle of encapsulation, better to make these variables protected memebers
	// and initialize them from passed dictionary as Get_Map argument of chosen values
	TSG_Rect				m_BBox, wmsReqBBox;

	bool					m_bReProj, m_bReProjBbox, m_bTransparent;

	int					m_sizeX, m_sizeY, m_blockSizeX, m_blockSizeY, m_ReProjMethod;

	wxString				m_Proj, m_Format, m_Layers, m_Styles, m_ServerUrl, m_ReProj, m_LayerProjDef;

	//! Method which downloads data from WMS server according to values of class members and members in m_Capabilities
	/*!
	  Information from GetCapabilities response are stored in m_Capabilities object.
	  Class members store chosen values of parameters for GetMap request.
	  \return path to file with downloaded data
	*/
	wxString				Get_Map			(void);

protected:

	//! Abstract method, which sends GetMap request to WMS server and fetches response into file
	/*!
	  \return path to file with downloaded raster
	*/
	virtual wxString			_Download		(void) = 0;

	//! If bounding box has to be reprojected to coordinate system of WMS request, this method reprojects it
	/*!
	If it is needed destination bbox points are transformed into wms projection and then bbox is created
	from extreme coordinates of the transformed points (m_ReProj -> m_Proj
	\return reprojected bbox
	*/
	TSG_Rect				_ComputeBbox		(void);

	//! Reprojection of raster into demanded projection (m_Proj -> m_ReProj) with m_ReProjMethod method modified code from http://www.gdal.org/warptut.html
	/*!
	  \return path to file with reprojected raster
	*/
	wxString				_GdalWarp		(const  wxString & tempMapPath);

};


//! Implements GDAL driver for GetMap request
class CWMS_Gdal_drv : public CWMS_Base
{

public:

	CWMS_Gdal_drv(): CWMS_Base() {}

private:

	//! Downloads raster with GetMap request using GDAL driver
	/*!
	  \return path to downloaded raster file
	*/
	wxString				_Download		(void);

	//! Creates XML file in format for GDAL driver 	with parameters for GetMap request
	/*!
	  \return path to XML file
	*/
	wxString				_CreateGdalDrvXml	(void);
};


//! It is one node of layer tree structure, which is created by getCapabilities Create() method
class CWMS_Layer
{
public:

	CWMS_Layer(void);

	virtual ~CWMS_Layer(void);

	void _Delete_Child_Layers(void);

	CWMS_Layer *                            m_pParrentLayer;

	std::vector<CWMS_Layer* >		m_ChildLayers;

	int	                                m_id;

	//! adds layer into layer tree
	/*!
	   Inititalizes m_pParrentLayer, m_ChildLayers, m_id and get all inherited features from parent layer.
	*/
	void					Create			(wxXmlNode * pNode, CWMS_Layer * pParrentLayer, const int layer_id, CWMS_XmlHandlers * xmlH, wxString m_ProjTag);

	/*!
	  \return  vector from m_LayerElms map item, which has key same as  elementsName. Returns empty vector if elementsName is not found in map.
	*/
	std::vector<wxXmlNode *>		getElms (const wxString & elementsName);


protected:


	//! All nodes of <Layer> attribute are saved there, including inherited nodes.
	/*!
	  For outer access use getElms method.
	*/
	std::map<wxString, std::vector<wxXmlNode*> >m_LayerElms;

	wxXmlNode *                             m_pLayerNode;

	wxString 				m_ProjTag;

	CWMS_XmlHandlers *			m_pXmlH;


	//! Checks if in m_pLayerNode is any child element, which has same value as inherited element from m_pParrentLayer
	/*!
	  Checked value depends on cmpType.
	  \param elementName - name of checked elements. The checked elements are children of m_pLayerNode
	  \param cmpType can hold this values: "attribute" - checked value is  node (elementName) attribute with name specified in addArg
					       "element_content" - checked value is  content of element (elementName)
					       "child_element_content" - checked value is  content of child element (specified in addArg)
	  \param inhElems - vector with already inherited elements, method will add elementName there

	*/
	void					_inhNotSameElement	(const wxString & elementName, const wxString & cmpType,
									 wxArrayString & inhElems, const wxString & addArg = wxT(""));
};

//! Stores projection and corresponding bounding box
class CWMS_Projection
{
public:

	CWMS_Projection(void)		{m_GeoBBox.xMax = m_GeoBBox.xMin = m_GeoBBox.yMax = m_GeoBBox.yMin = 0.0;}

	virtual ~CWMS_Projection(void)	{}

	TSG_Rect			m_GeoBBox;

	wxString			m_Projection;
};






#endif // #ifndef HEADER_INCLUDED__WMS_Import_H
