/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file  c3d_render_createscene_ogl_legacy.cpp
 * @brief
 */

#include "c3d_render_ogl_legacy.h"
#include "ogl_legacy_utils.h"
#include <class_board.h>
#include <class_module.h>
#include "../../3d_math.h"
#include "../../3d_fastmath.h"
#include <trigo.h>
#include <project.h>
#include <profile.h>        // To use GetRunningMicroSecs or another profiling utility


void C3D_RENDER_OGL_LEGACY::add_object_to_triangle_layer( const CFILLEDCIRCLE2D * aFilledCircle,
                                                          CLAYER_TRIANGLES *aDstLayer,
                                                          float aZtop,
                                                          float aZbot )
{
    const SFVEC2F &center = aFilledCircle->GetCenter();
    const float radius = aFilledCircle->GetRadius() *
                         2.0f; // Double because the render triangle

    // This is a small adjustment to the circle texture
    const float texture_factor = (8.0f / (float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;
    const float f = (sqrtf(2.0f) / 2.0f) * radius * texture_factor;

    // Top and Bot segments ends are just triangle semi-circles, so need to add
    // it in duplicated
    aDstLayer->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, aZtop ),
                                                      SFVEC3F( center.x - f, center.y, aZtop ),
                                                      SFVEC3F( center.x,
                                                               center.y - f, aZtop ) );

    aDstLayer->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, aZtop ),
                                                      SFVEC3F( center.x + f, center.y, aZtop ),
                                                      SFVEC3F( center.x,
                                                               center.y + f, aZtop ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, aZbot ),
                                                      SFVEC3F( center.x + f, center.y, aZbot ),
                                                      SFVEC3F( center.x,
                                                               center.y - f, aZbot ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, aZbot ),
                                                      SFVEC3F( center.x - f, center.y, aZbot ),
                                                      SFVEC3F( center.x,
                                                               center.y + f, aZbot ) );
}


void C3D_RENDER_OGL_LEGACY::add_object_to_triangle_layer( const CPOLYGON4PTS2D * aPoly,
                                                          CLAYER_TRIANGLES *aDstLayer,
                                                          float aZtop,
                                                          float aZbot )
{
    const SFVEC2F &v0 = aPoly->GetV0();
    const SFVEC2F &v1 = aPoly->GetV1();
    const SFVEC2F &v2 = aPoly->GetV2();
    const SFVEC2F &v3 = aPoly->GetV3();

    add_triangle_top_bot( aDstLayer, v0, v2, v1, aZtop, aZbot );
    add_triangle_top_bot( aDstLayer, v2, v0, v3, aZtop, aZbot );
}


void C3D_RENDER_OGL_LEGACY::generate_ring_contour( const SFVEC2F &aCenter,
                                                   float aInnerRadius,
                                                   float aOuterRadius,
                                                   unsigned int aNr_sides_per_circle,
                                                   std::vector< SFVEC2F > &aInnerContourResult,
                                                   std::vector< SFVEC2F > &aOuterContourResult,
                                                   bool aInvertOrder )
{
    aInnerContourResult.clear();
    aInnerContourResult.reserve( aNr_sides_per_circle + 2 );

    aOuterContourResult.clear();
    aOuterContourResult.reserve( aNr_sides_per_circle + 2 );

    const int delta = 3600 / aNr_sides_per_circle;

    for( int ii = 0; ii < 3600; ii += delta )
    {
        float angle = (float)( aInvertOrder ? ( 3600 - ii ) : ii )
                            * 2.0f * glm::pi<float>() / 3600.0f;
        const SFVEC2F rotatedDir = SFVEC2F( cos( angle ), sin( angle ) );

        aInnerContourResult.emplace_back( aCenter.x + rotatedDir.x * aInnerRadius,
                                                aCenter.y + rotatedDir.y * aInnerRadius );

        aOuterContourResult.emplace_back( aCenter.x + rotatedDir.x * aOuterRadius,
                                                aCenter.y + rotatedDir.y * aOuterRadius );
    }

    aInnerContourResult.push_back( aInnerContourResult[0] );
    aOuterContourResult.push_back( aOuterContourResult[0] );

    wxASSERT( aInnerContourResult.size() == aOuterContourResult.size() );
}


void C3D_RENDER_OGL_LEGACY::add_object_to_triangle_layer( const CRING2D * aRing,
                                                          CLAYER_TRIANGLES *aDstLayer,
                                                          float aZtop,
                                                          float aZbot )
{
    const SFVEC2F &center = aRing->GetCenter();
    const float inner = aRing->GetInnerRadius();
    const float outer = aRing->GetOuterRadius();

    std::vector< SFVEC2F > innerContour;
    std::vector< SFVEC2F > outerContour;

    generate_ring_contour( center,
                           inner,
                           outer,
                           m_boardAdapter.GetNrSegmentsCircle( outer * 2.0f ),
                           innerContour,
                           outerContour,
                           false );

    // This will add the top and bot quads that will form the approximated ring

    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
    {
        const SFVEC2F &vi0 = innerContour[i + 0];
        const SFVEC2F &vi1 = innerContour[i + 1];
        const SFVEC2F &vo0 = outerContour[i + 0];
        const SFVEC2F &vo1 = outerContour[i + 1];

        aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZtop ),
                                                   SFVEC3F( vi0.x, vi0.y, aZtop ),
                                                   SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                   SFVEC3F( vo1.x, vo1.y, aZtop ) );

        aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZbot ),
                                                   SFVEC3F( vo1.x, vo1.y, aZbot ),
                                                   SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                   SFVEC3F( vi0.x, vi0.y, aZbot ) );
    }
}


void C3D_RENDER_OGL_LEGACY::add_object_to_triangle_layer( const CTRIANGLE2D * aTri,
                                                          CLAYER_TRIANGLES *aDstLayer,
                                                          float aZtop,
                                                          float aZbot )
{
    const SFVEC2F &v1 = aTri->GetP1();
    const SFVEC2F &v2 = aTri->GetP2();
    const SFVEC2F &v3 = aTri->GetP3();

    add_triangle_top_bot( aDstLayer, v1, v2, v3, aZtop, aZbot );
}


void C3D_RENDER_OGL_LEGACY::add_object_to_triangle_layer( const CROUNDSEGMENT2D * aSeg,
                                                          CLAYER_TRIANGLES *aDstLayer,
                                                          float aZtop,
                                                          float aZbot )
{
    const SFVEC2F& leftStart   = aSeg->GetLeftStar();
    const SFVEC2F& leftEnd     = aSeg->GetLeftEnd();
    const SFVEC2F& leftDir     = aSeg->GetLeftDir();

    const SFVEC2F& rightStart  = aSeg->GetRightStar();
    const SFVEC2F& rightEnd    = aSeg->GetRightEnd();
    const SFVEC2F& rightDir    = aSeg->GetRightDir();
    const float   radius      = aSeg->GetRadius();

    const SFVEC2F& start       = aSeg->GetStart();
    const SFVEC2F& end         = aSeg->GetEnd();

    const float texture_factor = (12.0f / (float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;
    const float texture_factorF= ( 6.0f / (float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;

    const float radius_of_the_square = sqrtf( aSeg->GetRadiusSquared() * 2.0f );
    const float radius_triangle_factor = (radius_of_the_square - radius) / radius;

    const SFVEC2F factorS = SFVEC2F( -rightDir.y * radius * radius_triangle_factor,
                                      rightDir.x * radius * radius_triangle_factor );

    const SFVEC2F factorE = SFVEC2F( -leftDir.y  * radius * radius_triangle_factor,
                                      leftDir.x  * radius * radius_triangle_factor );

    // Top end segment triangles (semi-circles)
    aDstLayer->m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( rightEnd.x  + texture_factor * factorS.x,
                         rightEnd.y  + texture_factor * factorS.y,
                         aZtop ),
                SFVEC3F( leftStart.x + texture_factor * factorE.x,
                         leftStart.y + texture_factor * factorE.y,
                         aZtop ),
                SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf( 2.0f ),
                         start.y - texture_factorF * leftDir.y * radius * sqrtf( 2.0f ),
                         aZtop ) );

    aDstLayer->m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( leftEnd.x    + texture_factor * factorE.x,
                         leftEnd.y    + texture_factor * factorE.y, aZtop ),
                SFVEC3F( rightStart.x + texture_factor * factorS.x,
                         rightStart.y + texture_factor * factorS.y, aZtop ),
                SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf( 2.0f ),
                         end.y - texture_factorF * rightDir.y * radius * sqrtf( 2.0f ),
                         aZtop ) );

    // Bot end segment triangles (semi-circles)
    aDstLayer->m_layer_bot_segment_ends->AddTriangle(
                SFVEC3F( leftStart.x + texture_factor * factorE.x,
                         leftStart.y + texture_factor * factorE.y,
                         aZbot ),
                SFVEC3F( rightEnd.x  + texture_factor * factorS.x,
                         rightEnd.y  + texture_factor * factorS.y,
                         aZbot ),
                SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf( 2.0f ),
                         start.y - texture_factorF * leftDir.y * radius * sqrtf( 2.0f ),
                         aZbot ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle(
                SFVEC3F( rightStart.x + texture_factor * factorS.x,
                         rightStart.y + texture_factor * factorS.y, aZbot ),
                SFVEC3F( leftEnd.x    + texture_factor * factorE.x,
                         leftEnd.y    + texture_factor * factorE.y, aZbot ),
                SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf( 2.0f ),
                         end.y - texture_factorF * rightDir.y * radius * sqrtf( 2.0f ),
                         aZbot ) );

    // Segment top and bot planes
    aDstLayer->m_layer_top_triangles->AddQuad(
                SFVEC3F( rightEnd.x,   rightEnd.y,   aZtop ),
                SFVEC3F( rightStart.x, rightStart.y, aZtop ),
                SFVEC3F( leftEnd.x,    leftEnd.y,    aZtop ),
                SFVEC3F( leftStart.x,  leftStart.y,  aZtop ) );

    aDstLayer->m_layer_bot_triangles->AddQuad(
                SFVEC3F( rightEnd.x,   rightEnd.y,   aZbot ),
                SFVEC3F( leftStart.x,  leftStart.y,  aZbot ),
                SFVEC3F( leftEnd.x,    leftEnd.y,    aZbot ),
                SFVEC3F( rightStart.x, rightStart.y, aZbot ) );
}


CLAYERS_OGL_DISP_LISTS *C3D_RENDER_OGL_LEGACY::generate_holes_display_list(
        const LIST_OBJECT2D &aListHolesObject2d,
        const SHAPE_POLY_SET &aPoly,
        float aZtop,
        float aZbot,
        bool aInvertFaces )
{
    CLAYERS_OGL_DISP_LISTS *ret = NULL;

    if( aListHolesObject2d.size() > 0 )
    {
        CLAYER_TRIANGLES *layerTriangles = new CLAYER_TRIANGLES( aListHolesObject2d.size() * 2 );

        // Convert the list of objects(filled circles) to triangle layer structure
        for( LIST_OBJECT2D::const_iterator itemOnLayer = aListHolesObject2d.begin();
             itemOnLayer != aListHolesObject2d.end();
             ++itemOnLayer )
        {
            const COBJECT2D* object2d_A = static_cast<const COBJECT2D*>( *itemOnLayer );

            wxASSERT( ( object2d_A->GetObjectType() == OBJECT2D_TYPE::FILLED_CIRCLE )
                      || ( object2d_A->GetObjectType() == OBJECT2D_TYPE::ROUNDSEG ) );

            switch( object2d_A->GetObjectType() )
            {
            case OBJECT2D_TYPE::FILLED_CIRCLE:
                add_object_to_triangle_layer( static_cast<const CFILLEDCIRCLE2D*>( object2d_A ),
                        layerTriangles, aZtop, aZbot );
                break;

            case OBJECT2D_TYPE::ROUNDSEG:
                add_object_to_triangle_layer( static_cast<const CROUNDSEGMENT2D*>( object2d_A ),
                        layerTriangles, aZtop, aZbot );
                break;

            default:
                wxFAIL_MSG(
                        "C3D_RENDER_OGL_LEGACY::generate_holes_display_list: Object type is not implemented" );
                break;
            }
        }

        // Note: he can have a aListHolesObject2d whith holes but without countours
        // eg: when there are only NPTH on the list and the contours were not
        // added

        if( aPoly.OutlineCount() > 0 )
        {
            layerTriangles->AddToMiddleContourns( aPoly,
                                                  aZbot,
                                                  aZtop,
                                                  m_boardAdapter.BiuTo3Dunits(),
                                                  aInvertFaces );
        }

        ret = new CLAYERS_OGL_DISP_LISTS( *layerTriangles,
                                          m_ogl_circle_texture,
                                          aZbot,
                                          aZtop );

        delete layerTriangles;
    }

    return ret;
}


CLAYERS_OGL_DISP_LISTS* C3D_RENDER_OGL_LEGACY::generateLayerListFromContainer( const CBVHCONTAINER2D *aContainer,
                                                                               const SHAPE_POLY_SET *aPolyList,
                                                                               PCB_LAYER_ID aLayerId )
{
    if( aContainer == nullptr )
        return nullptr;

    const LIST_OBJECT2D &listObject2d = aContainer->GetList();

    if( listObject2d.size() == 0 )
        return nullptr;

    float layer_z_bot = 0.0f;
    float layer_z_top = 0.0f;

    get_layer_z_pos( aLayerId, layer_z_top, layer_z_bot );

    // Calculate an estimation for the nr of triangles based on the nr of objects
    unsigned int nrTrianglesEstimation = listObject2d.size() * 8;

    CLAYER_TRIANGLES *layerTriangles = new CLAYER_TRIANGLES( nrTrianglesEstimation );

    // store in a list so it will be latter deleted
    m_triangles.push_back( layerTriangles );

    // Load the 2D (X,Y axis) component of shapes
    for( LIST_OBJECT2D::const_iterator itemOnLayer = listObject2d.begin();
         itemOnLayer != listObject2d.end();
         ++itemOnLayer )
    {
        const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*itemOnLayer);

        switch( object2d_A->GetObjectType() )
        {
        case OBJECT2D_TYPE::FILLED_CIRCLE:
            add_object_to_triangle_layer( (const CFILLEDCIRCLE2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT2D_TYPE::POLYGON4PT:
            add_object_to_triangle_layer( (const CPOLYGON4PTS2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT2D_TYPE::RING:
            add_object_to_triangle_layer( (const CRING2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT2D_TYPE::TRIANGLE:
            add_object_to_triangle_layer( (const CTRIANGLE2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT2D_TYPE::ROUNDSEG:
            add_object_to_triangle_layer( (const CROUNDSEGMENT2D *) object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        default:
            wxFAIL_MSG("C3D_RENDER_OGL_LEGACY: Object type is not implemented");
            break;
        }
    }

    if( aPolyList )
        if( aPolyList->OutlineCount() > 0 )
            layerTriangles->AddToMiddleContourns( *aPolyList, layer_z_bot, layer_z_top,
                                                  m_boardAdapter.BiuTo3Dunits(), false );
    // Create display list
    // /////////////////////////////////////////////////////////////////////
    return new CLAYERS_OGL_DISP_LISTS( *layerTriangles,
                                       m_ogl_circle_texture,
                                       layer_z_bot,
                                       layer_z_top );
}


CLAYERS_OGL_DISP_LISTS* C3D_RENDER_OGL_LEGACY::createBoard( SHAPE_POLY_SET aBoardPoly )
{
    CLAYERS_OGL_DISP_LISTS* dispLists = nullptr;
    CCONTAINER2D boardContainer;
    Convert_shape_line_polygon_to_triangles( aBoardPoly,
                                             boardContainer,
                                             m_boardAdapter.BiuTo3Dunits(),
                                             (const BOARD_ITEM &)*m_boardAdapter.GetBoard() );

    const LIST_OBJECT2D &listBoardObject2d = boardContainer.GetList();

    if( listBoardObject2d.size() > 0 )
    {
        // We will set a unitary Z so it will in future used with transformations
        // since the board poly will be used not only to draw itself but also the
        // solder mask layers.
        const float layer_z_top = 1.0f;
        const float layer_z_bot = 0.0f;

        CLAYER_TRIANGLES *layerTriangles = new CLAYER_TRIANGLES( listBoardObject2d.size() );

        // Convert the list of objects(triangles) to triangle layer structure
        for( LIST_OBJECT2D::const_iterator itemOnLayer = listBoardObject2d.begin();
             itemOnLayer != listBoardObject2d.end();
             ++itemOnLayer )
        {
            const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*itemOnLayer);

            wxASSERT( object2d_A->GetObjectType() == OBJECT2D_TYPE::TRIANGLE );

            const CTRIANGLE2D *tri = (const CTRIANGLE2D *)object2d_A;

            const SFVEC2F &v1 = tri->GetP1();
            const SFVEC2F &v2 = tri->GetP2();
            const SFVEC2F &v3 = tri->GetP3();

            add_triangle_top_bot( layerTriangles,
                                  v1,
                                  v2,
                                  v3,
                                  layer_z_top,
                                  layer_z_bot );
        }

        const SHAPE_POLY_SET &boardPoly = m_boardAdapter.GetBoardPoly();

        wxASSERT( boardPoly.OutlineCount() > 0 );

        if( boardPoly.OutlineCount() > 0 )
        {
            layerTriangles->AddToMiddleContourns( boardPoly,
                                                  layer_z_bot,
                                                  layer_z_top,
                                                  m_boardAdapter.BiuTo3Dunits(),
                                                  false );

            dispLists = new CLAYERS_OGL_DISP_LISTS( *layerTriangles,
                                                    m_ogl_circle_texture,
                                                    layer_z_top,
                                                    layer_z_top );
        }

        delete layerTriangles;
    }

    return dispLists;
}


void C3D_RENDER_OGL_LEGACY::reload( REPORTER* aStatusReporter, REPORTER* aWarningReporter )
{
    m_reloadRequested = false;

    ogl_free_all_display_lists();

    COBJECT2D_STATS::Instance().ResetStats();

    unsigned stats_startReloadTime = GetRunningMicroSecs();

    m_boardAdapter.InitSettings( aStatusReporter, aWarningReporter );

    SFVEC3F camera_pos = m_boardAdapter.GetBoardCenter3DU();
    m_camera.SetBoardLookAtPos( camera_pos );

    if( aStatusReporter )
        aStatusReporter->Report( _( "Load OpenGL: board" ) );

    // Create Board
    // /////////////////////////////////////////////////////////////////////////

    m_ogl_disp_list_board = createBoard( m_boardAdapter.GetBoardPoly() );

    SHAPE_POLY_SET anti_board;
    anti_board.NewOutline();
    anti_board.Append( VECTOR2I( -INT_MAX/2, -INT_MAX/2 ) );
    anti_board.Append( VECTOR2I(  INT_MAX/2, -INT_MAX/2 ) );
    anti_board.Append( VECTOR2I(  INT_MAX/2,  INT_MAX/2 ) );
    anti_board.Append( VECTOR2I( -INT_MAX/2,  INT_MAX/2 ) );
    anti_board.Outline( 0 ).SetClosed( true );

    anti_board.BooleanSubtract( m_boardAdapter.GetBoardPoly(), SHAPE_POLY_SET::PM_FAST );
    m_ogl_disp_list_anti_board = createBoard( anti_board );
    m_ogl_disp_list_anti_board->SetItIsTransparent( true );

    // Create Through Holes and vias
    // /////////////////////////////////////////////////////////////////////////

    if( aStatusReporter )
        aStatusReporter->Report( _( "Load OpenGL: holes and vias" ) );

    m_ogl_disp_list_through_holes_outer = generate_holes_display_list(
            m_boardAdapter.GetThroughHole_Outer().GetList(),
            m_boardAdapter.GetThroughHole_Outer_poly(),
            1.0f,
            0.0f,
            false );

    SHAPE_POLY_SET bodyHoles = m_boardAdapter.GetThroughHole_Outer_poly();

    bodyHoles.BooleanAdd( m_boardAdapter.GetThroughHole_Outer_poly_NPTH(),
                          SHAPE_POLY_SET::PM_FAST );

    m_ogl_disp_list_through_holes_outer_with_npth = generate_holes_display_list(
            m_boardAdapter.GetThroughHole_Outer().GetList(),
            bodyHoles,
            1.0f,
            0.0f,
            false );

    m_ogl_disp_list_through_holes_vias_outer = generate_holes_display_list(
            m_boardAdapter.GetThroughHole_Vias_Outer().GetList(),
            m_boardAdapter.GetThroughHole_Vias_Outer_poly(),
            1.0f,
            0.0f,
            false );

    if( m_boardAdapter.GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) )
    {
        m_ogl_disp_list_through_holes_outer_ring = generate_holes_display_list(
                m_boardAdapter.GetThroughHole_Outer_Ring().GetList(),
                m_boardAdapter.GetThroughHole_Outer_Ring_poly(),
                1.0f,
                0.0f,
                false );
    }

    // Not in use
    //m_ogl_disp_list_through_holes_vias_inner = generate_holes_display_list(
    //      m_boardAdapter.GetThroughHole_Vias_Inner().GetList(),
    //      m_boardAdapter.GetThroughHole_Vias_Inner_poly(),
    //      1.0f, 0.0f,
    //      false );

    const MAP_POLY & innerMapHoles = m_boardAdapter.GetPolyMapHoles_Inner();
    const MAP_POLY & outerMapHoles = m_boardAdapter.GetPolyMapHoles_Outer();

    wxASSERT( innerMapHoles.size() == outerMapHoles.size() );

    const MAP_CONTAINER_2D &map_holes = m_boardAdapter.GetMapLayersHoles();

    if( outerMapHoles.size() > 0 )
    {
        float layer_z_bot = 0.0f;
        float layer_z_top = 0.0f;

        for( MAP_POLY::const_iterator ii = outerMapHoles.begin();
             ii != outerMapHoles.end();
             ++ii )
        {
            PCB_LAYER_ID layer_id = static_cast<PCB_LAYER_ID>(ii->first);
            const SHAPE_POLY_SET *poly = static_cast<const SHAPE_POLY_SET *>(ii->second);
            const CBVHCONTAINER2D *container = map_holes.at( layer_id );

            get_layer_z_pos( layer_id, layer_z_top, layer_z_bot );

            m_ogl_disp_lists_layers_holes_outer[layer_id] = generate_holes_display_list(
                        container->GetList(), *poly, layer_z_top, layer_z_bot, false );
        }

        for( MAP_POLY::const_iterator ii = innerMapHoles.begin();
             ii != innerMapHoles.end();
             ++ii )
        {
            PCB_LAYER_ID layer_id = static_cast<PCB_LAYER_ID>(ii->first);
            const SHAPE_POLY_SET *poly = static_cast<const SHAPE_POLY_SET *>(ii->second);
            const CBVHCONTAINER2D *container = map_holes.at( layer_id );

            get_layer_z_pos( layer_id, layer_z_top, layer_z_bot );

            m_ogl_disp_lists_layers_holes_inner[layer_id] = generate_holes_display_list(
                        container->GetList(), *poly, layer_z_top, layer_z_bot, false );
        }
    }

    // Generate vertical cylinders of vias and pads (copper)
    generate_3D_Vias_and_Pads();

    // Add layers maps

    if( aStatusReporter )
        aStatusReporter->Report( _( "Load OpenGL: layers" ) );

    const MAP_POLY &map_poly = m_boardAdapter.GetPolyMap();

    for( MAP_CONTAINER_2D::const_iterator ii = m_boardAdapter.GetMapLayers().begin();
         ii != m_boardAdapter.GetMapLayers().end();
         ++ii )
    {
        PCB_LAYER_ID layer_id = static_cast<PCB_LAYER_ID>(ii->first);

        if( !m_boardAdapter.Is3DLayerEnabled( layer_id ) )
            continue;

        const CBVHCONTAINER2D *container2d = static_cast<const CBVHCONTAINER2D *>(ii->second);

        // Load the vertical (Z axis) component of shapes
        const SHAPE_POLY_SET *aPolyList = nullptr;

        if( map_poly.find( layer_id ) != map_poly.end() )
        {
            aPolyList = map_poly.at( layer_id );
        }

        CLAYERS_OGL_DISP_LISTS* oglList = generateLayerListFromContainer( container2d, aPolyList, layer_id );

        if( oglList != nullptr )
            m_ogl_disp_lists_layers[layer_id] = oglList;

    }// for each layer on

    if( m_boardAdapter.GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) )
    {
        m_ogl_disp_lists_platedPads_F_Cu = generateLayerListFromContainer( m_boardAdapter.GetPlatedPads_Front(),
                                                                           m_boardAdapter.GetPolyPlatedPads_Front(), F_Cu );
        m_ogl_disp_lists_platedPads_B_Cu = generateLayerListFromContainer( m_boardAdapter.GetPlatedPads_Back(),
                                                                           m_boardAdapter.GetPolyPlatedPads_Back(), B_Cu );
    }

    // Load 3D models
    // /////////////////////////////////////////////////////////////////////////
    if( aStatusReporter )
        aStatusReporter->Report( _( "Loading 3D models" ) );

    load_3D_models( aStatusReporter );

    if( aStatusReporter )
    {
        // Calculation time in seconds
        const double calculation_time = (double)( GetRunningMicroSecs() -
                                                  stats_startReloadTime) / 1e6;

        aStatusReporter->Report( wxString::Format( _( "Reload time %.3f s" ), calculation_time ) );
    }
}


void C3D_RENDER_OGL_LEGACY::add_triangle_top_bot( CLAYER_TRIANGLES *aDst,
                                                  const SFVEC2F &v0,
                                                  const SFVEC2F &v1,
                                                  const SFVEC2F &v2,
                                                  float top,
                                                  float bot )
{
    aDst->m_layer_bot_triangles->AddTriangle( SFVEC3F( v0.x, v0.y, bot ),
                                              SFVEC3F( v1.x, v1.y, bot ),
                                              SFVEC3F( v2.x, v2.y, bot ) );

    aDst->m_layer_top_triangles->AddTriangle( SFVEC3F( v2.x, v2.y, top ),
                                              SFVEC3F( v1.x, v1.y, top ),
                                              SFVEC3F( v0.x, v0.y, top ) );
}


void C3D_RENDER_OGL_LEGACY::get_layer_z_pos ( PCB_LAYER_ID aLayerID,
                                              float &aOutZtop,
                                              float &aOutZbot ) const
{
    aOutZbot = m_boardAdapter.GetLayerBottomZpos3DU( aLayerID );
    aOutZtop = m_boardAdapter.GetLayerTopZpos3DU( aLayerID );

    if( aOutZtop < aOutZbot )
    {
        float tmpFloat = aOutZbot;
        aOutZbot = aOutZtop;
        aOutZtop = tmpFloat;
    }
}


void C3D_RENDER_OGL_LEGACY::generate_cylinder( const SFVEC2F &aCenter,
                                               float aInnerRadius,
                                               float aOuterRadius,
                                               float aZtop,
                                               float aZbot,
                                               unsigned int aNr_sides_per_circle,
                                               CLAYER_TRIANGLES *aDstLayer )
{
    std::vector< SFVEC2F > innerContour;
    std::vector< SFVEC2F > outerContour;

    generate_ring_contour( aCenter,
                           aInnerRadius,
                           aOuterRadius,
                           aNr_sides_per_circle,
                           innerContour,
                           outerContour,
                           false );

    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
    {
        const SFVEC2F &vi0 = innerContour[i + 0];
        const SFVEC2F &vi1 = innerContour[i + 1];
        const SFVEC2F &vo0 = outerContour[i + 0];
        const SFVEC2F &vo1 = outerContour[i + 1];

        aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZtop ),
                                                   SFVEC3F( vi0.x, vi0.y, aZtop ),
                                                   SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                   SFVEC3F( vo1.x, vo1.y, aZtop ) );

        aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZbot ),
                                                   SFVEC3F( vo1.x, vo1.y, aZbot ),
                                                   SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                   SFVEC3F( vi0.x, vi0.y, aZbot ) );
    }

    aDstLayer->AddToMiddleContourns( outerContour, aZbot, aZtop, true );
    aDstLayer->AddToMiddleContourns( innerContour, aZbot, aZtop, false );
}


void C3D_RENDER_OGL_LEGACY::generate_3D_Vias_and_Pads()
{
    if( m_boardAdapter.GetStats_Nr_Vias() )
    {
        const unsigned int reserve_nr_triangles_estimation =
                m_boardAdapter.GetNrSegmentsCircle( m_boardAdapter.GetStats_Med_Via_Hole_Diameter3DU() ) *
                8 *
                m_boardAdapter.GetStats_Nr_Vias();

        CLAYER_TRIANGLES *layerTriangleVIA = new CLAYER_TRIANGLES( reserve_nr_triangles_estimation );

        // Insert plated vertical holes inside the board
        // /////////////////////////////////////////////////////////////////////////

        // Insert vias holes (vertical cylinders)
        for( auto track : m_boardAdapter.GetBoard()->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                const VIA *via = static_cast<const VIA*>(track);

                const float  holediameter = via->GetDrillValue() * m_boardAdapter.BiuTo3Dunits();
                const float  thickness = m_boardAdapter.GetCopperThickness3DU();
                const int    nrSegments = m_boardAdapter.GetNrSegmentsCircle( via->GetDrillValue() );
                const float  hole_inner_radius = holediameter / 2.0f;

                const SFVEC2F via_center( via->GetStart().x * m_boardAdapter.BiuTo3Dunits(),
                                          -via->GetStart().y * m_boardAdapter.BiuTo3Dunits() );

                PCB_LAYER_ID top_layer, bottom_layer;
                via->LayerPair( &top_layer, &bottom_layer );

                float ztop, zbot, dummy;

                get_layer_z_pos( top_layer,    ztop,  dummy );
                get_layer_z_pos( bottom_layer, dummy, zbot );

                wxASSERT( zbot < ztop );

                generate_cylinder( via_center,
                                   hole_inner_radius,
                                   hole_inner_radius + thickness,
                                   ztop,
                                   zbot,
                                   nrSegments,
                                   layerTriangleVIA );
            }
        }

        m_ogl_disp_list_via = new CLAYERS_OGL_DISP_LISTS( *layerTriangleVIA,
                                                          0,
                                                          0.0f,
                                                          0.0f );

        delete layerTriangleVIA;
    }


    if( m_boardAdapter.GetStats_Nr_Holes() > 0 )
    {
        SHAPE_POLY_SET tht_outer_holes_poly; // Stores the outer poly of the copper holes (the pad)
        SHAPE_POLY_SET tht_inner_holes_poly; // Stores the inner poly of the copper holes (the hole)

        tht_outer_holes_poly.RemoveAllContours();
        tht_inner_holes_poly.RemoveAllContours();

        // Insert pads holes (vertical cylinders)
        for( const auto module : m_boardAdapter.GetBoard()->Modules() )
        {
            for( auto pad : module->Pads() )
            {
                if( pad->GetAttribute() != PAD_ATTRIB_NPTH )
                {
                    const wxSize drillsize = pad->GetDrillSize();
                    const bool   hasHole   = drillsize.x && drillsize.y;

                    if( !hasHole )
                        continue;

                    const int copperThickness = m_boardAdapter.GetHolePlatingThicknessBIU();

                    pad->TransformHoleWithClearanceToPolygon( tht_outer_holes_poly,
                                                              copperThickness, ARC_LOW_DEF );
                    pad->TransformHoleWithClearanceToPolygon( tht_inner_holes_poly,
                                                              0, ARC_LOW_DEF );
                }
            }
        }

        // Subtract the holes
        tht_outer_holes_poly.BooleanSubtract( tht_inner_holes_poly, SHAPE_POLY_SET::PM_FAST );


        CCONTAINER2D holesContainer;

        Convert_shape_line_polygon_to_triangles( tht_outer_holes_poly,
                                                 holesContainer,
                                                 m_boardAdapter.BiuTo3Dunits(),
                                                 (const BOARD_ITEM &)*m_boardAdapter.GetBoard() );

        const LIST_OBJECT2D &listHolesObject2d = holesContainer.GetList();

        if( listHolesObject2d.size() > 0 )
        {
            float layer_z_top, layer_z_bot, dummy;

            get_layer_z_pos( F_Cu, layer_z_top,  dummy );
            get_layer_z_pos( B_Cu, dummy, layer_z_bot );

            CLAYER_TRIANGLES *layerTriangles = new CLAYER_TRIANGLES( listHolesObject2d.size() );

            // Convert the list of objects(triangles) to triangle layer structure
            for( LIST_OBJECT2D::const_iterator itemOnLayer = listHolesObject2d.begin();
                 itemOnLayer != listHolesObject2d.end();
                 ++itemOnLayer )
            {
                const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*itemOnLayer);

                wxASSERT( object2d_A->GetObjectType() == OBJECT2D_TYPE::TRIANGLE );

                const CTRIANGLE2D *tri = (const CTRIANGLE2D *)object2d_A;

                const SFVEC2F &v1 = tri->GetP1();
                const SFVEC2F &v2 = tri->GetP2();
                const SFVEC2F &v3 = tri->GetP3();

                add_triangle_top_bot( layerTriangles, v1, v2, v3,
                                      layer_z_top, layer_z_bot );
            }

            wxASSERT( tht_outer_holes_poly.OutlineCount() > 0 );

            if( tht_outer_holes_poly.OutlineCount() > 0 )
            {
                layerTriangles->AddToMiddleContourns( tht_outer_holes_poly,
                                                      layer_z_bot, layer_z_top,
                                                      m_boardAdapter.BiuTo3Dunits(),
                                                      false );

                m_ogl_disp_list_pads_holes = new CLAYERS_OGL_DISP_LISTS(
                            *layerTriangles,
                            m_ogl_circle_texture, // not need
                            layer_z_top, layer_z_top );
            }

            delete layerTriangles;
        }
    }
}


/*
 * This function will get models from the cache and load it to openGL lists
 * in the form of C_OGL_3DMODEL. So this map of models will work as a local
 * cache for this render. (cache based on C_OGL_3DMODEL with associated
 * openGL lists in GPU memory)
 */
void C3D_RENDER_OGL_LEGACY::load_3D_models( REPORTER* aStatusReporter )
{
    if((!m_boardAdapter.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL )) &&
       (!m_boardAdapter.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT )) &&
       (!m_boardAdapter.GetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL )) )
        return;

    // Go for all modules
    for( MODULE* module : m_boardAdapter.GetBoard()->Modules() )
    {
        for( const MODULE_3D_SETTINGS& model : module->Models() )
        {
            if( model.m_Show && !model.m_Filename.empty() )
            {
                if( aStatusReporter )
                {
                    // Display the short filename of the 3D model loaded:
                    // (the full name is usually too long to be displayed)
                    wxFileName fn( model.m_Filename );
                    wxString msg;
                    msg.Printf( _( "Loading %s" ), fn.GetFullName() );
                    aStatusReporter->Report( msg );
                }

                // Check if the model is not present in our cache map
                // (Not already loaded in memory)
                if( m_3dmodel_map.find( model.m_Filename ) == m_3dmodel_map.end() )
                {
                    // It is not present, try get it from cache
                    const S3DMODEL* modelPtr =
                            m_boardAdapter.Get3DCacheManager()->GetModel( model.m_Filename );

                    // only add it if the return is not NULL
                    if( modelPtr )
                    {
                        MATERIAL_MODE materialMode = m_boardAdapter.MaterialModeGet();
                        C_OGL_3DMODEL* ogl_model = new C_OGL_3DMODEL( *modelPtr, materialMode );

                        if( ogl_model )
                            m_3dmodel_map[ model.m_Filename ] = ogl_model;
                    }
                }
            }
        }
    }
}
