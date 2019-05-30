#include "..\dlx\clone_dlx.hpp"

#ifndef MISC_INCLUDED
	#include <misc.hxx>
#endif
#ifndef FUNC_INCLUDED
	#include <func.hxx>
#endif

#include <uf.h>
#include <uf_assem.h>
#include <uf_disp.h>

#include <ug_session.hxx>
//#include <ug_part.hxx> // <-----------------  #incude's bug

#include <sstream>

//*****************************************************************************

iSession::iSession ( const NXOpen::Session * sess ) : NXOpen::Session (*sess) {
	UgSession ugsess(true);
	try
	{
		UGMGR_PART P ( this->Parts()->Work()->Tag() );
		wp.part_tag = P.tag;
		wp.part_number = P.pn;
		wp.part_revision = P.pr;
		wp.part_file_name = P.pfn;
		wp.part_file_type = P.pft;
		wp.encoded_name = P.encoded_name;
		wp.key = wp.part_file_type + "*" + wp.part_file_name + "*" + wp.part_number + "*" + wp.part_revision;
		wp.name = Parts()->Work()->GetStringAttribute("DB_PART_NAME").GetLocaleText();
		wp.desc = Parts()->Work()->GetStringAttribute("DB_PART_DESC").GetLocaleText();

		UF_FREE_STRING FS;
		EXEC ( UF_UGMGR_convert_file_name_to_cli( P.encoded_name, &FS.ch ) );
		wp.cli_name = FS.ch;	
	}
	BASE_ERR_PRINT; // iSession
};

//******************************************************************************

ClonePart::ClonePart ( const tag_t part_occ ) {
	UgSession ugsess(true);
	try
	{
		if ( UF_ASSEM_is_part_occurrence ( part_occ ) )
		{
			part_occ_tag = part_occ;
			part_tag = UF_ASSEM_ask_prototype_of_occ ( part_occ_tag );
			if ( !part_tag ) { Ms( "Выбранный компонент не загружен" ); throw EXIT(); }
			UGMGR_PART P ( part_tag );
			P.load();
			part_number = P.pn;
			part_revision = P.pr;
			part_file_name = P.pfn;
			part_file_type = P.pft;
			this->encoded_name = P.encoded_name;
			key = part_file_type + "*" + part_file_name + "*" + part_number + "*" + part_revision;

			UF_FREE_STRING FS;
			EXEC ( UF_UGMGR_convert_file_name_to_cli( P.encoded_name, &FS.ch ) );
			cli_name = FS.ch;	

			NXOpen::Part * nxpart = (NXOpen::Part*)NXOpen::NXObjectManager::Get (part_tag);
			name = nxpart->GetStringAttribute("DB_PART_NAME").GetLocaleText();
			desc = nxpart->GetStringAttribute("DB_PART_DESC").GetLocaleText();

			tag_t parent = UF_ASSEM_ask_part_occurrence ( part_occ ); level = 0;
			while ( parent ) {
				level++; parent = UF_ASSEM_ask_part_occurrence ( parent );
			}
		}		
	}
	BASE_ERR_THROW;
};

void ClonePart::decode_after_key ( std::string input_string )
{
	UgSession ugsess(true);
	try
	{
		std::string::iterator it = input_string.begin();
		for ( ; it != input_string.end(); it++ ) if (*it == '*') { *it = '\n'; }
		
		std::stringstream inpt_s;
		inpt_s << input_string;

		getline( inpt_s, after_part_file_type );
		getline( inpt_s, after_part_file_name );
		getline( inpt_s, after_part_number );
		getline( inpt_s, after_part_revision );

		char encoded_name_ [ MAX_FSPEC_SIZE+1 ] = {0};
		EXEC ( UF_UGMGR_encode_part_filename ( after_part_number.c_str(), after_part_revision.c_str(), after_part_file_type.c_str(), after_part_file_name.c_str(), encoded_name_ )  );
		after_encoded_name = encoded_name_;

		UF_FREE_STRING FS;
		EXEC ( UF_UGMGR_convert_file_name_to_cli( after_encoded_name.c_str(), &FS.ch ) );
		after_cli_name = FS.ch;
	}
	BASE_ERR_THROW;
	return;
};

void ClonePart::decode_part_file_name ( char * code )
{
	UgSession ugsess(true);
	try
	{
		char part_number [ UF_UGMGR_PARTNO_SIZE+1 ]={0}, part_revision [ UF_UGMGR_PARTREV_SIZE+1 ]={0}, part_file_type [ UF_UGMGR_FTYPE_SIZE+1 ]={0}, part_file_name [ UF_UGMGR_FNAME_SIZE+1 ]={0};
		if ( !std::string(code).empty() )
			EXEC ( UF_UGMGR_decode_part_file_name ( code, part_number, part_revision, part_file_type, part_file_name ) );
		after_part_file_name = part_file_name;
		after_part_number = part_number;
		after_part_revision = part_revision;
		after_part_file_type = part_file_type;
		after_key = after_part_file_type + "*" + after_part_file_name + "*" + after_part_number + "*" + after_part_revision;
		
		UF_FREE_STRING FS, FS_internal;
		EXEC ( UF_UGMGR_convert_file_name_to_cli( code, &FS.ch ) );
		after_cli_name = FS.ch;
		
		if ( !after_cli_name.empty() ) {
			EXEC ( UF_UGMGR_convert_name_from_cli ( FS.ch, &FS_internal.ch ) );
			after_encoded_name = FS_internal.ch;
		} else
			after_encoded_name = code;

	}
	BASE_ERR_THROW;
	return;
};

void ClonePart::assign_part_rev ( )
{
	UgSession ugsess(true);
	try
	{
		after_part_number = part_number;

		char part_rev [ UF_UGMGR_PARTREV_SIZE+1 ]  = {0};
		logical modifiable = FALSE;
		EXEC ( UF_UGMGR_assign_part_rev ( after_part_number.c_str(), NULL, part_rev, &modifiable ) );
		after_part_revision = part_rev;
		after_part_file_type = part_file_type;
		after_part_file_name = part_file_name;

		char encoded_name [ MAX_FSPEC_SIZE+1 ] = {0};
		EXEC ( UF_UGMGR_encode_part_filename ( after_part_number.c_str(), after_part_revision.c_str(), after_part_file_type.c_str(), after_part_file_name.c_str(), encoded_name )  );
		decode_part_file_name( encoded_name );

	}
	BASE_ERR_THROW;
	return;
};

//***********************************************************************************
#include <Windows.h>
void iCloneBilder::saveasNonmaster ( ClonePart * cp )
{
	UgSession ugsess(true);
	try
	{
		std::string input_str = cp->part_number + "\n";
		input_str += cp->part_revision + "\n";
		input_str += cp->part_file_type + "\n";
		input_str += cp->part_file_name + "\n";
		input_str += cp->after_part_number + "\n";
		input_str += cp->after_part_revision + "\n";
		input_str += cp->after_part_file_type + "\n";
		input_str += cp->after_part_file_name;

		
		int	output_code = 0;
		UF_FREE_STRING   output_string;

		EXEC ( UF_UGMGR_invoke_pdm_server ( 3004, const_cast < char * > (input_str.c_str()),  &output_code, &output_string.ch ));
		if ( output_code ) { Message( output_string.ch, 0 ); }
		else {
			Sleep(500);
			Ms( "Сохранение завершено. Необходимо открыть в NX сохранённые данные" );
		}
/*		LOADSTATUS LS;

		tag_t wp = UF_ASSEM_ask_work_part(), opened_part = NULL_TAG, prev_wp = NULL_TAG;

		EXEC ( UF_PART_open_quiet ( cp->after_encoded_name.c_str(), &opened_part, &LS.error_status ) );
		EXEC ( UF_ASSEM_set_work_part_quietly ( opened_part, &prev_wp ) );
		EXEC (UF_PART_save_work_only ( ) );
		EXEC ( UF_ASSEM_set_work_part_quietly ( wp, &prev_wp ) );
		EXEC (UF_PART_close( opened_part,1,1 ));
*/
	}
	BASE_ERR_THROW;
	return;
};

//fd
void write_lw (std::string what);
std::string get_error_text ( int code );

void iCloneBilder::reopen ()
{
	UgSession ugsess(true);
	try
	{
		write_lw( "Closing all..." );
		int err = UF_PART_close_all();
		LOADSTATUS LS;
		std::string what = Clone::theSession->wp.after_encoded_name.empty()? Clone::theSession->wp.encoded_name : Clone::theSession->wp.after_encoded_name.c_str();
		std::string report_str = Clone::theSession->wp.after_encoded_name.empty()? Clone::theSession->wp.cli_name : Clone::theSession->wp.after_cli_name.c_str();
		write_lw( std::string("Reopen ") + report_str );
		err = UF_PART_open ( what.c_str(), &Clone::theSession->wp.part_tag, &LS.error_status );
		write_lw(std::string("Reopening is finished. Error code: ") + toString(err) + get_error_text(err) );
	}
	BASE_ERR_THROW;
	return;
};

void iCloneBilder::clone_children ()
{
	UgSession ugsess(true);
	try
	{
		int err = 0;
		tag_t previous_work_part = NULL_TAG;
		for ( std::vector < ClonePart * >::iterator pos = clone_it.begin(); pos != clone_it.end(); pos++ ) 
		{
			EXEC ( UF_ASSEM_set_work_part_quietly ( (*pos)->part_tag, &previous_work_part ) );
			write_lw(std::string("Cloning part: ") + (*pos)->cli_name.c_str() );
			err = UF_PART_save_as ( (*pos)->after_encoded_name.c_str() );
			write_lw(std::string("Cloning is finished. Error code: ") + toString(err) + get_error_text(err) );
			if ( !err ) be_reopened = true;
			write_lw("Setting database attributes...");
			set_attributes ( *pos );
		}
	}
	BASE_ERR_THROW;
	return;
};

void iCloneBilder::save_children ()
{
	UgSession ugsess(true);
	try
	{
		int err = 0;
		logical writable = false;
		tag_t previous_work_part = NULL_TAG;
		for ( std::vector < ClonePart * >::iterator pos = save_it.begin(); pos != save_it.end(); pos++ ) 
		{
			EXEC ( UF_PART_check_part_writable ( (*pos)->encoded_name.c_str(), &writable ) );
			if ( !writable ) {
				write_lw(std::string("Saving part: ") + (*pos)->cli_name.c_str() + " - failed: no write access" );
				continue;
			}

			EXEC ( UF_ASSEM_set_work_part_quietly ( (*pos)->part_tag, &previous_work_part ) );
			write_lw(std::string("Saving part: ") + (*pos)->cli_name.c_str() );
			err = UF_PART_save_work_only ( );
			write_lw(std::string("Saving is finished. Error code: ") + toString(err) + get_error_text(err) );
		}
	}
	BASE_ERR_THROW;
	return;
};

void iCloneBilder::work_part_processing()
{
	UgSession ugsess(true);
	try
	{
		if ( UF_ASSEM_ask_work_part() != Clone::theSession->wp.part_tag ) UF_ASSEM_set_work_part ( Clone::theSession->wp.part_tag );

		int err = 0;
		if ( Clone::theSession->wp.action == ClonePart::ActionClone ) {
			write_lw(std::string("Cloning part: ") + Clone::theSession->wp.cli_name.c_str() );
			err = UF_PART_save_as ( Clone::theSession->wp.after_encoded_name.c_str() );
			write_lw("Setting database attributes...");
			set_attributes ( &Clone::theSession->wp );
			write_lw(std::string("Cloning is finished. Error code: ") + toString(err) + get_error_text(err) );
			be_reopened = false;
		}
		else if ( Clone::theSession->wp.action == ClonePart::ActionRetain ) {
			logical writable = false;
			EXEC ( UF_PART_check_part_writable ( Clone::theSession->wp.encoded_name.c_str(), &writable ) );
			if ( !writable ) {
				write_lw(std::string("Saving part: ") + Clone::theSession->wp.cli_name.c_str() + " - failed: no write access" );
			} else {
				write_lw(std::string("Saving part: ") + Clone::theSession->wp.cli_name.c_str() );
				err = UF_PART_save ( );
				write_lw(std::string("Saving is finished. Error code: ") + toString(err) + get_error_text(err) );
				be_reopened = false;
			}
		}
/*		else if ( Clone::theSession->wp.action == ClonePart::ActionSaveAs ) {
			write_lw(std::string("->Save As Non-Master. Part: ") + Clone::theSession->wp.cli_name.c_str() );
			err = UF_PART_save_work_only ( );
			saveasNonmaster ( &Clone::theSession->wp );
			write_lw(std::string("Saving is finished. Error code: ") + toString(err) + get_error_text(err) );
			be_reopened = true;
		}
*/
	}
	BASE_ERR_THROW;
	return;
};

void iCloneBilder::sort( std::vector < ClonePart * > & me )
{
	try
	{
		std::vector < ClonePart * > temp;
		std::vector < ClonePart * >::iterator pos, it = me.begin();

		for ( ; it != me.end(); it++ ) {
			ClonePart * cp = *it;
			// sorting by level 5 4 3 3 2 2 2 1 1 1 1
			pos = temp.begin();
			for ( ; pos != temp.end(); pos++  ) {
				if ( (*pos)->level <= cp->level ) break;
			}
			temp.insert(pos,cp);
		}
		
		me.clear();
		me.assign(temp.begin(), temp.end());

		if ( me.size() != temp.size() ) { Message( "Sorting Failed", 0 ); throw EXIT(); }
	}
	BASE_ERR_THROW;
	return;
};

void iCloneBilder::set_new_part_type(  )
{
	UgSession ugsess(true);
	try
	{
		int (*UGMGR__set_new_part_type)(char**) = NULL;
		char lib_name[_MAX_PATH] = {0};
		_searchenv(   "libugmr.dll", "UGII_ROOT_DIR", lib_name );

		std::string exp_f_name = "?UGMGR_ask_default_part_type@@YAHPAPAD@Z";
//		std::string exp_f_name = "?UGMGR__set_new_part_type@@YAXPBD@Z";
//		if ( ug_ver == std::string ( "UG_VER_NX4X64" ) )
//			exp_f_name = "?MACRO_playback_from_usertool@@YAXPEBD@Z";

		EXEC ( UF_load_library ( lib_name, const_cast < char * > ( exp_f_name.c_str()), (UF_load_f_p_t *)&UGMGR__set_new_part_type ));

		UF_FREE_STRING F;
		if (UGMGR__set_new_part_type) UGMGR__set_new_part_type(&F.ch);
		Ms(F.ch);

	}
	BASE_ERR_THROW;
	return;
};

void iCloneBilder::set_attributes( ClonePart * cp )
{
	UgSession ugsess(true);
	try
	{
		std::string input_str = cp->after_part_number + "\n";
		input_str += cp->after_part_revision + "\n";
		input_str += std::string("ItemRevision") + "\n";
		input_str += std::string("string") + "\n";
		input_str += std::string("object_name") + "\n";
		input_str += cp->after_name;
		
		int	output_code = 0;
		UF_FREE_STRING   output_string;

		if ( !cp->after_name.empty() )
			EXEC ( UF_UGMGR_invoke_pdm_server ( 2604, const_cast < char * > (input_str.c_str()),  &output_code, &output_string.ch ));
		if ( output_code ) { Message( output_string.ch, 0 ); }

		input_str.clear();
		input_str = cp->after_part_number + "\n";
		input_str += cp->after_part_revision + "\n";
		input_str += std::string("ItemRevision") + "\n";
		input_str += std::string("string") + "\n";
		input_str += std::string("object_desc") + "\n";
		input_str += cp->after_desc;
		
		UF_FREE_STRING   output_string2;

		if ( !cp->after_desc.empty() )
			EXEC ( UF_UGMGR_invoke_pdm_server ( 2604, const_cast < char * > (input_str.c_str()),  &output_code, &output_string2.ch ));
		if ( output_code ) { Message( output_string2.ch, 0 ); }

		UF_UGMGR_tag_t database_part_tag = NULL_TAG;
		EXEC ( UF_UGMGR_ask_part_tag ( const_cast <char*> (cp->after_part_number.c_str()), &database_part_tag ) );
		
		AUTOTAG A1;
		EXEC ( UF_UGMGR_list_part_revisions ( database_part_tag, &A1.num, &A1.tags ) );
		if ( A1.num == 1 )
		{
			if ( !cp->after_name.empty() && !cp->after_desc.empty() )
				EXEC ( UF_UGMGR_set_part_name_desc ( database_part_tag, const_cast <char*>(cp->after_name.c_str()), const_cast <char*>(cp->after_desc.c_str()) ));
		}
	}
	BASE_ERR_THROW;
	return;
};

void ClonePart::do_substitution ()
{
	UgSession ugsess(true);
	try
	{
		// prevent tag change after substitution
		part_tag = UF_PART_ask_part_tag ( encoded_name.c_str() );

		// load parent if needed
		tag_t parent = UF_ASSEM_ask_part_occurrence ( part_occ_tag );
		while ( parent ) {
			tag_t proto = UF_ASSEM_ask_prototype_of_occ ( parent );
			if ( proto ) {UGMGR_PART P (proto); if (P.load_status==2)P.load(); }
			parent = UF_ASSEM_ask_part_occurrence ( parent );
		}
		AUTOTAG A1;
		A1.num = UF_ASSEM_ask_occs_of_part ( NULL_TAG, part_tag, &A1.tags );
		EXEC ( UF_PART_close ( part_tag, 1, 1 ) );
				
		tag_t opened_part = NULL_TAG;
		LOADSTATUS LS, LSO;

		int err = UF_PART_open_single_component_as ( part_occ_tag, after_encoded_name.c_str(), &opened_part, &LS.error_status );
		if ( err ) {
			//if ( err == 1 )  Message ( "Замена невозможна: Parent not fully loaded, or component has no parent", 0 );
			if ( err == 1 )  Message ( "Замена невозможна: Родительская подсборка не загружена полностью!", 0 );
			if ( err == 2 )  Message ( "Замена невозможна: New part can't be loaded", 0 );
			if ( err == 3 )  Message ( "Замена невозможна: Old part is already loaded", 0 );
			//if ( err == 4 ) throw std::string ( "New part not substitutable for old part (uid's must match)" );
			if ( err == 4 ) { Message ( "Замена невозможна: Для замены назначен неподходящий компонент (uid's must match)", 0 ); }
			if ( err > 4 )  Message ( "Замена невозможна: Unspecified error", 0 );
			EXEC ( UF_PART_open_quiet ( encoded_name.c_str(), &part_tag, &LSO.error_status ) );
			EXEC ( UF_DISP_regenerate_display () );
			throw EXIT();
		}
				
		if ( A1.num > 1 ) EXEC ( UF_PART_open_quiet ( encoded_name.c_str(), &part_tag, &LSO.error_status ) );
		EXEC ( UF_DISP_regenerate_display () );
	}
	BASE_ERR_THROW;
	return;
};
