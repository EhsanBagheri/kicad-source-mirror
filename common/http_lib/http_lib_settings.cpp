/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <nlohmann/json.hpp>

#include <settings/parameters.h>
#include <wildcards_and_files_ext.h>

#include <http_lib/http_lib_settings.h>

const int httplibSchemaVersion = 1;


HTTP_LIB_SETTINGS::HTTP_LIB_SETTINGS( const std::string& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, httplibSchemaVersion )
{
    m_params.emplace_back( new PARAM<std::string>( "source.type", &sourceType, "" ) );

    m_params.emplace_back(
            new PARAM<std::string>( "source.api_version", &m_Source.api_version, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.root_url", &m_Source.root_url, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.token", &m_Source.token, "" ) );
}


wxString HTTP_LIB_SETTINGS::getFileExt() const
{
    return HTTPLibraryFileExtension;
}
