/*****************************************************************************
**
** Moments_of_Inertia_toexp_ufusr.cpp
**
** Description:
**     Contains Unigraphics entry points for the application.
**
*****************************************************************************/

/* Include files */

#include <NXOpen/Session.hxx>
#include <NXOpen/Part.hxx>
#include <NXOpen/PartCollection.hxx>
#include <NXOpen/Callback.hxx>
#include <NXOpen/NXException.hxx>
#include <NXOpen/UI.hxx>
#include <NXOpen\NXMessageBox.hxx>
#include <NXOpen/Selection.hxx>
#include <NXOpen/LogFile.hxx>
#include <NXOpen/NXObjectManager.hxx>
#include <NXOpen/ListingWindow.hxx>
#include <NXOpen/View.hxx>
#include <NXOpen/DisplayManager.hxx>
#include <NXOpen/Features_FeatureCollection.hxx>
#include <NXOpen/Point.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedClass.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedClassManager.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedObject.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedObjectManager.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedEvent.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedDisplayEvent.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedLinkEvent.hxx>
#include <NXOpen/UserDefinedObjects_UserDefinedObjectDisplayContext.hxx>
#include <NXOpen/Features_UserDefinedObjectFeatureBuilder.hxx>
#include <NXOpen/Expression.hxx>
#include <NXOpen/ExpressionCollection.hxx>

#include <uf.h>
#include <ug_session.hxx>
#include <ug_exception.hxx>
#include <uf_ui.h>
#include <uf_exit.h>
#include <ug_info_window.hxx>
#include <uf_weight.h>
#include <uf_udobj.h>
#include <uf_obj.h>
#include <uf_so.h>
#include <uf_modl.h>

#include <stdarg.h>

#include <string>
#include <iostream>
#include <sstream>

static void ECHO(char *format, ...)
{
    char msg[UF_UI_MAX_STRING_LEN+1];
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);
    UF_UI_open_listing_window();
    UF_UI_write_listing_window(msg);
    UF_print_syslog(msg, FALSE);
}

#define UF_CALL(X) (report_error( __FILE__, __LINE__, #X, (X)))

static int report_error( char *file, int line, char *call, int irc)
{
    if (irc)
    {
        char err[133];

        UF_get_fail_message(irc, err);
        ECHO("*** ERROR code %d at line %d in %s:\n",
            irc, line, file);
        ECHO("+++ %s\n", err);
        ECHO("%s;\n", call);
    }

    return(irc);
}

std::string toString ( double d ){
	std::stringstream s; s << d;
	return s.str();
};

static UF_UDOBJ_class_t mi2exp_class_id = 0;

DllExport extern UF_UDOBJ_class_t get_mi2exp_class_id(void)
{
    return mi2exp_class_id;
}

DllExport extern void mi2exp_update_cb ( tag_t udo, UF_UDOBJ_link_p_t update_cause )
{
	UgSession sess(true);
	try
	{
		UF_UDOBJ_all_data_t all_data;
		UF_CALL(UF_UDOBJ_ask_udo_data (udo, &all_data));
		tag_t solid_body = all_data.link_defs[0].assoc_ug_tag,
		se_mi_xx = all_data.link_defs[1].assoc_ug_tag,
		se_mi_yy = all_data.link_defs[2].assoc_ug_tag,
		se_mi_zz = all_data.link_defs[3].assoc_ug_tag;
		UF_CALL(UF_UDOBJ_free_udo_data( &all_data ));

		tag_t c_exp_xx, c_exp_yy, c_exp_zz;
		UF_CALL( UF_SO_ask_exp_of_scalar ( se_mi_xx, &c_exp_xx ) );
		UF_CALL( UF_SO_ask_exp_of_scalar ( se_mi_yy, &c_exp_yy ) );
		UF_CALL( UF_SO_ask_exp_of_scalar ( se_mi_zz, &c_exp_zz ) );

		UF_WEIGHT_properties_t props;
			UF_CALL ( UF_WEIGHT_estab_solid_props ( solid_body, 0.99, UF_WEIGHT_units_km, &props )  );

		NXOpen::Expression * exp_xx = ( NXOpen::Expression* )NXOpen::NXObjectManager::Get(c_exp_xx);
		NXOpen::Expression * exp_yy = ( NXOpen::Expression* )NXOpen::NXObjectManager::Get(c_exp_yy);
		NXOpen::Expression * exp_zz = ( NXOpen::Expression* )NXOpen::NXObjectManager::Get(c_exp_zz);

		if ( exp_xx ) exp_xx->SetValue(props.moments_of_inertia[0]);
		if ( exp_yy ) exp_yy->SetValue(props.moments_of_inertia[1]);
		if ( exp_zz ) exp_zz->SetValue(props.moments_of_inertia[2]);

		UF_MODL_update ();
	}
	catch ( ... ) { NXOpen::UI::GetUI()->NXMessageBox()->Show( "Error", NXOpen::NXMessageBox::DialogTypeError, "Something else..." ); }
	return;
}
DllExport extern void mi2exp_delete_cb ( tag_t udo, UF_UDOBJ_link_p_t update_cause )
{
	UgSession sess(true);
	try
	{
		UF_UDOBJ_all_data_t all_data;
		UF_CALL(UF_UDOBJ_ask_udo_data (udo, &all_data));
		tag_t assoc_obj[4] = { all_data.link_defs[0].assoc_ug_tag, all_data.link_defs[1].assoc_ug_tag,all_data.link_defs[2].assoc_ug_tag,
			                   all_data.link_defs[3].assoc_ug_tag };
		UF_CALL(UF_UDOBJ_free_udo_data( &all_data ));
		tag_t exp;
		logical is_so;
		for( int i=0; i<4;i++ ) {
			UF_CALL( UF_SO_is_so( assoc_obj[i], &is_so ) );
			if ( is_so ) {
				UF_CALL( UF_SO_ask_exp_of_scalar ( assoc_obj[i], &exp )  );
				UF_CALL( UF_SO_delete_parms (assoc_obj[i] ) );
				NXOpen::Session::GetSession()->Parts()->Work()->Expressions()->Delete(dynamic_cast<NXOpen::Expression*>(NXOpen::NXObjectManager::Get(exp)));
			}
		}
	}
	catch ( ... ) { NXOpen::UI::GetUI()->NXMessageBox()->Show( "Error", NXOpen::NXMessageBox::DialogTypeError, "Something else..." ); }
}

static void create_udo_class(std::string class_name)
{
	UgSession session( true );
	try
	{
		UF_CALL(UF_UDOBJ_create_class( const_cast <char*>(class_name.c_str()), const_cast<char*>((class_name+"_friendly_name").c_str()), &mi2exp_class_id));
		UF_CALL ( UF_UDOBJ_set_query_class_id ( mi2exp_class_id, UF_UDOBJ_ALLOW_QUERY_CLASS_ID ) );
		UF_CALL(UF_UDOBJ_register_update_cb( mi2exp_class_id, mi2exp_update_cb ));
		UF_CALL ( UF_UDOBJ_register_delete_cb ( mi2exp_class_id, mi2exp_delete_cb ) );
	}
	catch ( ... ) { NXOpen::UI::GetUI()->NXMessageBox()->Show( "Error", NXOpen::NXMessageBox::DialogTypeError, "Something else..." ); }
	return;
};

class iAutoTag
{
public:
	iAutoTag():num(0), tags(0){};
	~iAutoTag(){ if (tags) UF_free(tags);tags=0; num=0; };

	int num;
	UF_UDOBJ_link_t * tags;
};

class iAutoString
{
public:
	iAutoString(): str(0){};
	~iAutoString(){if(str) UF_free(str); str=0;};

	char * str;
};

/*****************************************************************************
**  Activation Methods
*****************************************************************************/
/*  Explicit Activation
**      This entry point is used to activate the application explicitly, as in
**      "File->Execute UG/Open->User Function..." */

extern DllExport void ufusr( char *parm, int *returnCode, int rlen )
{
    UgSession session( true );

    try
    {
		NXOpen::Session * theSession = NXOpen::Session::GetSession();
		NXOpen::BasePart* theBasePart = theSession->Parts()->BaseDisplay();
        if( theBasePart == NULL) return;

        NXOpen::NXObject* theObj = NULL;
        NXOpen::Point3d theCursor(0,0,0);

        // ask the user to select a solid body to calculate moments of inertia to link to UDO
        std::vector<NXOpen::Selection::MaskTriple> mask(1);
        mask[0] = NXOpen::Selection::MaskTriple( UF_solid_type, 0, 0  );

		NXOpen::Selection::Response theResponse = NXOpen::UI::GetUI()->SelectionManager()->SelectObject("Select Solid Body", "UDO Link Target", 
              NXOpen::Selection::SelectionScopeWorkPart, NXOpen::Selection::SelectionActionClearAndEnableSpecific, FALSE, FALSE, mask, &theObj, &theCursor);
        if ( theResponse != NXOpen::Selection::ResponseCancel )
        {
			UF_UDOBJ_class_t mi2exp_udo_class = get_mi2exp_class_id();

			if (mi2exp_udo_class == 0)
			{
				create_udo_class("mi2exp_udo");
				mi2exp_udo_class = get_mi2exp_class_id();
			}

			UF_WEIGHT_properties_t props;
			UF_CALL ( UF_WEIGHT_estab_solid_props ( theObj->Tag(), 0.99, UF_WEIGHT_units_km, &props )  );
			/*
			2.2	Moments of Inertia (Centroidal)
				This moments of inertia are around the coordinate system whose axes are parallel to WCS, but the origin is at the centroid of the solid.
				*/

			iAutoTag UDO;
			UF_CALL ( UF_UDOBJ_ask_udo_links_to_obj ( theObj->Tag(), &UDO.num, &UDO.tags )  );
			for ( int i=0; i<UDO.num; i++ )
			{
				iAutoString udo_class_name;
				UF_CALL ( UF_UDOBJ_ask_udo_class_name ( UDO.tags[i].assoc_ug_tag, &udo_class_name.str ) );
				std::string udo_class_name_str = udo_class_name.str;
				if ( udo_class_name_str == std::string("mi2exp_udo") ) {
					NXOpen::UI::GetUI()->NXMessageBox()->Show( "Information", NXOpen::NXMessageBox::DialogTypeInformation, "The UDO object already exists" );
					return;
				}
			}

			tag_t XX_exp_tag = 0, YY_exp_tag = 0, ZZ_exp_tag = 0;
			UF_CALL ( UF_MODL_create_exp_tag( (std::string("XX_mom_of_inertia=")+toString(props.moments_of_inertia[0])).c_str(), &XX_exp_tag ) );
			UF_CALL ( UF_MODL_create_exp_tag( (std::string("YY_mom_of_inertia=")+toString(props.moments_of_inertia[1])).c_str(), &YY_exp_tag ) );
			UF_CALL ( UF_MODL_create_exp_tag( (std::string("ZZ_mom_of_inertia=")+toString(props.moments_of_inertia[2])).c_str(), &ZZ_exp_tag ) );
			
			tag_t mi_xx = NULL_TAG, mi_yy = NULL_TAG, mi_zz = NULL_TAG;
			UF_CALL ( UF_SO_create_scalar_exp ( theObj->Tag(), UF_SO_update_within_modeling, XX_exp_tag, &mi_xx ) );
			UF_CALL ( UF_SO_create_scalar_exp ( theObj->Tag(), UF_SO_update_within_modeling, YY_exp_tag, &mi_yy ) );
			UF_CALL ( UF_SO_create_scalar_exp ( theObj->Tag(), UF_SO_update_within_modeling, ZZ_exp_tag, &mi_zz ) );
			
			if ( !mi_xx || !mi_yy || !mi_zz ) throw NXOpen::NXException::Create ( "Failed to create the scalar" );


				tag_t udo = NULL_TAG;
				UF_UDOBJ_link_t link_defs[4];
				link_defs[0].link_type = 1;
				link_defs[0].assoc_ug_tag = theObj->Tag();
				link_defs[0].object_status = 0;
				link_defs[1].link_type = 1;
				link_defs[1].assoc_ug_tag = mi_xx;
				link_defs[1].object_status = 0;
				link_defs[2].link_type = 1;
				link_defs[2].assoc_ug_tag = mi_yy;
				link_defs[2].object_status = 0;
				link_defs[3].link_type = 1;
				link_defs[3].assoc_ug_tag = mi_zz;
				link_defs[3].object_status = 0;
	
				UF_CALL ( UF_UDOBJ_create_udo ( mi2exp_udo_class, &udo ) );
				UF_CALL ( UF_UDOBJ_add_links ( udo, 4, link_defs ) );

				NXOpen::UI::GetUI()->NXMessageBox()->Show("Information", NXOpen::NXMessageBox::DialogTypeInformation, "The UDO object successfully created");
				std::cout << "The UDO object successfully created" << std::endl;
		}
    }
	catch ( ... ) { NXOpen::UI::GetUI()->NXMessageBox()->Show( "Error", NXOpen::NXMessageBox::DialogTypeError, "Something else..." ); }
	return;
}


extern int ufusr_ask_unload( void )
{
    return( UF_UNLOAD_UG_TERMINATE ); // !
}
