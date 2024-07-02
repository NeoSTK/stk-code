//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2011-2015 Marianne Gagnon
//  based on code Copyright 2002-2010 Nikolaus Gebhardt
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

#include "graphics/camera/camera.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/lod_node.hpp"
#include "config/user_config.hpp"
#include "karts/abstract_kart.hpp"

#include <ISceneManager.h>
#include <ICameraSceneNode.h>
#include <IMeshSceneNode.h>
#include <IAnimatedMeshSceneNode.h>

/**
  * @param group_name Only useful for getGroupName()
  */
LODNode::LODNode(std::string group_name, scene::ISceneNode* parent,
                 scene::ISceneManager* mgr, s32 id)
    : ISceneNode(parent, mgr, id)
{
    m_update_box_every_frame = false;
    assert(mgr != NULL);
    assert(parent != NULL);

    m_group_name = group_name;

    // At this stage refcount is two: one because of the object being
    // created, and once because it is a child of the parent. Drop once,
    // so that only the reference from the parent is active, causing this
    // node to be deleted when it is removed from the parent.
    drop();

    m_forced_lod = -1;
    m_current_level = -1;
    m_current_level_dirty = true;
    m_lod_distances_updated = false;
    m_min_switch_distance = 0;
}

LODNode::~LODNode()
{
}

void LODNode::render()
{
    //ISceneNode::render();
}

/** Returns the level to use, or -1 if the object is too far
 *  away.
 */
int LODNode::getLevel()
{
    if (m_nodes.size() == 0)
        return -1;

    // If a level is forced, use it
    if (m_forced_lod > -1)
        return m_forced_lod;
    
    if (!m_current_level_dirty)
        return m_current_level;
    m_current_level_dirty = false;

    Camera* camera = Camera::getActiveCamera();
    if (camera == NULL)
        return (int)m_detail.size() - 1;
    const Vec3 &pos = camera->getCameraSceneNode()->getAbsolutePosition();

    const int squared_dist =
        (int)((m_nodes[0]->getAbsolutePosition()).getDistanceFromSQ(pos.toIrrVector() ));

    if (!m_lod_distances_updated)
    {
        for (unsigned int n=0; n<m_detail.size(); n++)
        {
            m_detail[n] = (int)((float)m_detail[n] * irr_driver->getLODMultiplier());
        }
        m_lod_distances_updated = true;
    }

    // The LoD levels are ordered from highest quality to lowest
    for (unsigned int n=0; n<m_detail.size(); n++)
    {
        // Display this level if close enough.
        // If a high-level of detail would only be triggered from too close,
        // and there are lower levels available, skip it completely.
        // It's better to display a low level than to have the high-level pop suddenly.
        if (squared_dist < m_detail[n] &&
            ((n + 1 >= m_detail.size()) || (m_min_switch_distance < m_detail[n])))
        {
            m_current_level = n;
            return n;
        }
    }
    m_current_level = m_detail.size();
    return m_detail.size();
}  // getLevel

// ---------------------------------------------------------------------------
/** Forces the level of detail to be n. If n>number of levels, the most
 *  detailed level is used. This is used to disable LOD when the end
 *  camera is activated, since it zooms in to the kart. */
void LODNode::forceLevelOfDetail(int n)
{
    m_forced_lod = n;
}   // forceLevelOfDetail

// ----------------------------------------------------------------------------
void LODNode::OnAnimate(u32 timeMs)
{
    if (isVisible() && m_nodes.size() > 0)
    {
        // update absolute position
        updateAbsolutePosition();

        for (size_t i = 0; i < m_nodes.size(); i++)
        {
            m_nodes[i]->setVisible(true);
            m_nodes[i]->OnAnimate(timeMs);
        }

        if (m_update_box_every_frame)
            Box = m_nodes[m_detail.size() - 1]->getBoundingBox();

        // If this node has children other than the LOD nodes, animate it
        for (unsigned i = 0; i < Children.size(); ++i)
        {
            if (m_nodes_set.find(Children[i]) == m_nodes_set.end())
            {
                assert(Children[i] != NULL);
                if (Children[i]->isVisible())
                {
                    Children[i]->OnAnimate(timeMs);
                }
            }
        }
    } // if isVisible() && m_nodes.size() > 0
}

void LODNode::updateVisibility()
{
    if (!isVisible()) return;
    if (m_nodes.size() == 0) return;

    m_current_level_dirty = true;
    unsigned int level = getLevel();

    for (size_t i = 0; i < m_nodes.size(); i++)
    {
        m_nodes[i]->setVisible(i == level);
    }
}

void LODNode::OnRegisterSceneNode()
{
    updateVisibility();

#ifndef SERVER_ONLY
    if (CVS->isGLSL())
    {
        return;
    }
#endif

    if (isVisible() && m_nodes.size() > 0)
    {
        int level = getLevel();
        
        if (level < m_detail.size())
        {
            m_nodes[level]->OnRegisterSceneNode();
        }
        for (unsigned i = 0; i < Children.size(); i++)
        {
            if (m_nodes_set.find(Children[i]) == m_nodes_set.end())
                Children[i]->OnRegisterSceneNode();
        }
    } // if isVisible() && m_nodes.size() > 0
}

void LODNode::add(int level, scene::ISceneNode* node, bool reparent)
{
    Box = node->getBoundingBox();

    // samuncle suggested to put a slight randomisation in LOD
    // I'm not convinced (Auria) but he's the artist pro, so I listen ;P
    // The last level should not be randomized because after that the object disappears,
    // and the location is disapparition needs to be deterministic
    if (m_detail.size() > 0 && m_detail.back() < level * level)
    {
        m_detail[m_detail.size() - 1] += (int)(((rand()%1000)-500)/500.0f*(m_detail[m_detail.size() - 1]*0.2f));
    }

    assert(node != NULL);

    node->grab();
    node->remove();
    node->setPosition(core::vector3df(0,0,0));
    m_detail.push_back(level*level);
    m_nodes.push_back(node);
    m_nodes_set.insert(node);
    node->setParent(this);

    node->drop();

    node->updateAbsolutePosition();
    node->setNeedsUpdateAbsTrans(true);
}
