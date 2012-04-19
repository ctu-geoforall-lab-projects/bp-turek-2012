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
//                      wms_import.h                     //
//                                                       //
//                 Copyright (C) 2011 by                 //
//                      Olaf Conrad                      //
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
#ifndef HEADER_INCLUDED__WMS_Import_H
#define HEADER_INCLUDED__WMS_Import_H


///////////////////////////////////////////////////////////
//														 //
//														 //
//														 //
///////////////////////////////////////////////////////////

//---------------------------------------------------------

#include <gdal.h>

#include "MLB_Interface.h"
#include <vector>
#include <list>

#include "wms_import.h"

#include <wx/protocol/http.h>

///////////////////////////////////////////////////////////
//														 //
//														 //
//														 //
///////////////////////////////////////////////////////////


class CWMS_Layer;
class CWMS_Style;
class CWMS_XmlHandlers;
class CWMS_WMS_Base;
class CWMS_WMS_Gdal_drv;
class CWMS_Projection;

class CWMS_Import : public CSG_Module
{
public:
	friend class CWMS_WMS_Base;

	friend class CWMS_WMS_Gdal_drv;//TODO

	CWMS_Import(void);

	virtual ~CWMS_Import();

protected:

	CWMS_WMS_Base *				m_WMS;

	bool					Create_Layers_Dialog	( std::vector<CWMS_Layer*> layers, CSG_Parameters & parLayers);

	bool					Create_Settings_Dialog	( std::vector<CWMS_Layer*> selectedLayers, CSG_Parameters & parSettings);

	static int				_On_Layer_Changed	(CSG_Parameter *pParameter, int Flags);

	virtual bool				On_Execute		(void);

};

class CWMS_Capabilities;

class CWMS_Capabilities
{
public:
	CWMS_Capabilities(void);

	CWMS_Capabilities(CWMS_WMS_Base * pParrentWMS);

	virtual ~CWMS_Capabilities(void);


	bool				Create			( CWMS_WMS_Base * pParrentWMS);

	CSG_String			Get_Summary		(void);


	int				m_MaxLayers, m_MaxWidth, m_MaxHeight;//TODO max constaints

	CSG_String			m_Name, m_Title, m_Abstract, m_Online, m_Contact, m_Fees, m_Access, m_Formats, m_ProjTag, m_Version;

	CSG_Strings			m_Keywords;

	CWMS_Layer			*m_RootLayer;

	wxHTTP				m_Server;

	CWMS_WMS_Base *			m_ParrentWMS;

	CSG_Strings 			Proj_Intersect		(std::vector<CWMS_Layer* > & layers );

	void				Get_Layers		(CWMS_Layer * layer, std::vector<CWMS_Layer* > & layers);

private:

	bool				_Get_Capabilities	(class wxXmlNode *pRoot);

	void				_Reset			(void);

	bool                            _Load_Layers		(class wxXmlNode *pNode, int & layer_id  ,CWMS_Layer *parrent_layer = NULL, int nDepth = 0);


};

class CWMS_WMS_Base
{
public:

	CWMS_WMS_Base(void){}

	virtual ~CWMS_WMS_Base(void){}

	//bool Create( ); reset; TODO friend cappp then hide createcap????

	//bool Create(class wxHTTP *pServer, const CSG_String &Directory, CSG_String &Version);
	CWMS_Capabilities			m_Capabilities;

	bool					m_bReProj;

	int					m_sizeX, m_sizeY, m_blockSizeX, m_blockSizeY;// nastavit uvnitr tridy??

	CSG_String				m_Proj, m_Format, m_Layers, m_Styles, m_ServerUrl, m_Directory, m_ReProj;


	TSG_Rect				m_BBox, wmsReqBBox;

	CWMS_Import *				modul;//TODO realy needed????

	CSG_Parameter *				m_pOutputMap;

	bool					Get_Map			( void );

protected:

	virtual CSG_String			_Download		( void ) = 0;

	bool					_ComputeBbox		(void);

	bool					_CreateOutputMap	( CSG_String & tempMapPath );

	CSG_String				_CreateGdalDrvXml	( void );

	void	_GdalWarp		(  CSG_String & tempMapPath );//TODO virtual

	//virtual bool getMap( void );

};

class CWMS_WMS_Gdal_drv : public CWMS_WMS_Base
{

public:

	CWMS_WMS_Gdal_drv(): CWMS_WMS_Base() {}

private:

	CSG_String				_Download		( void );

	CSG_String				_CreateGdalDrvXml	( void );
};






class CWMS_Layer
{
public:

	CWMS_Layer(void);

	virtual ~CWMS_Layer(void)	{ _Delete_Child_Layers();}

	void _Delete_Child_Layers(void);


	CSG_String				m_Name, m_Title, m_Abstract, m_Version;

	CSG_Strings				m_Dimensions, m_KeywordList;

	int					m_MaxScaleD, m_MinScaleD, m_id, m_Cascaded, m_fixedWidth, m_fixedHeight;

	bool					m_Opaque, m_Querryable, no_Subsets;

	std::vector<class CWMS_Layer* >		 m_Layers;

	CWMS_Layer				*m_pParrentLayer;



	bool					Add_Style	( wxXmlNode * pProjNode);

	bool					Add_Proj	( wxXmlNode * pProjNode);

	bool					Add_GeoBBox	( wxXmlNode * pBboxNode, CSG_String projTag );


	bool					Assign_Styles	(std::vector<class CWMS_Style> styles) {m_Styles = styles;}

	bool					Assign_Projs	(std::vector<class CWMS_Projection> projections ) {m_Projections = projections;}


	std::vector<class CWMS_Style> &		Get_Styles	(void)	{ return m_Styles;}

	std::vector<class CWMS_Projection> &	Get_Projections	(void)	{ return  m_Projections;}


	bool					Create		(wxXmlNode * pNode, CWMS_Layer * pParrentLayer, const int layer_id, CSG_String m_ProjTag);

private:

	std::vector<class CWMS_Style>           m_Styles;

	std::vector<class CWMS_Projection>	m_Projections;


};


class CWMS_Projection
{
public:

	CWMS_Projection(void)		{}

	virtual ~CWMS_Projection(void)	{}

	TSG_Rect			m_GeoBBox;

	CSG_String			m_Projection;
};

class CWMS_Style
{
public:

	CWMS_Style(void)		{}

	virtual ~CWMS_Style(void)	{}

	CSG_String			m_Title;

	CSG_String			m_Name;
};


class CWMS_XmlHandlers
{
public:

	static class wxXmlNode *		_Get_Child		(class wxXmlNode *pNode, const CSG_String &Name);

	static bool				_Get_Child_Content	(class wxXmlNode *pNode, CSG_String &Value, const CSG_String &Name);

	static bool				_Get_Child_Content	(class wxXmlNode *pNode, int        &Value, const CSG_String &Name);

	static bool				_Get_Child_Content	(class wxXmlNode *pNode, double     &Value, const CSG_String &Name);

	static bool				_Get_Node_PropVal	(class wxXmlNode *pNode, CSG_String &Value, const CSG_String &Property);

	static bool				_Get_Child_PropVal	(class wxXmlNode *pNode, CSG_String &Value, const CSG_String &Name, const CSG_String &Property);

	static CSG_Strings			_Get_Children_Content	(class wxXmlNode *pNode, const CSG_String &children_name);
};

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
#endif // #ifndef HEADER_INCLUDED__WMS_Import_H
