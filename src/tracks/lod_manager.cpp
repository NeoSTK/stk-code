//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2015 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2010-2015 Steve Baker, Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "tracks/lod_manager.hpp"

#include "config/user_config.hpp"
#include "graphics/camera/camera.hpp"
#include "utils/mckp_solver.hpp"

#include "ICameraSceneNode.h"
#include "IMeshBuffer.h"
#include "SViewFrustum.h"

LODManager *lod_manager = NULL;

bool isBoxInFrontOfPlane(const core::plane3df &plane,
                        const core::vector3df* edges)
{
    for (u32 i = 0; i < 8; i++)
    {
        if (plane.classifyPointRelation(edges[i]) != core::ISREL3D_FRONT)
            return false;
    }
    return true;
}   // isBoxInFrontOfPlane

bool isCulledPrecise(const scene::ICameraSceneNode *cam,
                    const scene::ISceneNode* node)
{
    const core::matrix4 &trans = node->getAbsoluteTransformation();
    core::vector3df edges[8];
    node->getBoundingBox().getEdges(edges);
    for (unsigned i = 0; i < 8; i++)
        trans.transformVect(edges[i]);

    const scene::SViewFrustum &frust = *cam->getViewFrustum();
    for (s32 i = 0; i < scene::SViewFrustum::VF_PLANE_COUNT; i++)
    {
        if (isBoxInFrontOfPlane(frust.planes[i], edges))
        {
            return true;
        }
    }
    return false;

}   // isCulledPrecise

//-----------------------------------------------------------------------------
void LODManager::registerNode(LODNode* node, float importance, int* children_triangle_count)
{
    m_lod_nodes.push_back(node);
    m_importance.push_back(importance);

    m_children_triangle_count.push_back(std::vector<int>());
    m_children_triangle_count.back().resize(node->getAllNodes().size());

    for (int i = 0; i < node->getAllNodes().size(); i++)
    {
        m_children_triangle_count.back()[i] = children_triangle_count[i];
    }
    m_max_capacity += m_children_triangle_count.back()[0] + ObjectCapacityOffset;
}

//-----------------------------------------------------------------------------
void LODManager::autoComputeLevel()
{
    float capacity = m_max_capacity;
    capacity += OverallCapacityOffset;

    if(     UserConfigParams::m_geometry_level == 2) capacity *= 0.15f; // 2 in the params is the lowest setting
    else if(UserConfigParams::m_geometry_level == 1) capacity *= 0.3f;
    else if(UserConfigParams::m_geometry_level == 0) capacity *= 0.45f;
    else if(UserConfigParams::m_geometry_level == 3) capacity *= 0.6f;
    else if(UserConfigParams::m_geometry_level == 4) capacity *= 0.75f;
    else if(UserConfigParams::m_geometry_level == 5) capacity *= 0.9f;

    MCKPSolver solver(capacity, m_lod_nodes.size());

    Camera* camera = Camera::getActiveCamera();
    if (camera == NULL)
        return;
    const Vec3 &pos = camera->getCameraSceneNode()->getAbsolutePosition();

    for (int i = 0; i < m_lod_nodes.size(); i++)
    {
        float importance = m_importance[i];
        float radius_squared =
            m_lod_nodes[i]->getBoundingBox().getExtent().getLengthSQ() / 4.0f;
        float dist_squared =
            m_lod_nodes[i]->getAbsolutePosition().getDistanceFromSQ(pos.toIrrVector());
        float real_dist_squared =
            std::max(dist_squared, radius_squared);
        float bbox_area = 
            m_lod_nodes[i]->getBoundingBox().getArea();
        float real_dist = sqrtf(real_dist_squared);
        
        for (int j = 0; j < m_lod_nodes[i]->getAllNodes().size(); j++)
        {
            float value = 1.0f, weight = m_children_triangle_count[i][j] + ObjectCapacityOffset;

            // Model Accuracy Factor
            value *= std::max(0.0f, 1.0f - 1.0f / m_children_triangle_count[i][j]);

            // Importance Factor
            value *= importance;

            // Projection Size Factor
            value *= bbox_area / real_dist_squared;

            // Distance Factor
            value /= real_dist;

            // Hysteresis Offset
            if (j == m_lod_nodes[i]->getLevel() && !isCulledPrecise(camera->getCameraSceneNode(), m_lod_nodes[i]))
            {
                value += 0.2f * bbox_area / real_dist_squared / real_dist / importance;
            }

            solver.pushItem(value, weight, i);
        }
        float value = 0.0f;
        // Hysteresis Offset
        if (m_lod_nodes[i]->getAllNodes().size() == m_lod_nodes[i]->getLevel() && !isCulledPrecise(camera->getCameraSceneNode(), m_lod_nodes[i]))
        {
            value += 1.0f * bbox_area / real_dist_squared / real_dist / importance;
        }
        
        solver.pushItem(value, 0.0f, i); // Empty level
    }
    solver.solve();

    for (int i = 0; i < m_lod_nodes.size(); i++)
    {
        m_lod_nodes[i]->forceLevelOfDetail(solver.getChosen()[i]);
    }
}

//-----------------------------------------------------------------------------
void LODManager::clear()
{
    m_lod_nodes.clear();
    m_importance.clear();
    m_children_triangle_count.clear();
    m_max_capacity = 0;
}