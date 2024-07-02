//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2024 CodingJellyfish
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

#include "utils/mckp_solver.hpp"

#include <algorithm>
#include <assert.h>

const float Epsilon = 1e-6;


MCKPSolver::MCKPSolver(float capacity, int group_count) : m_capacity(capacity)
{
    m_all_items.resize(group_count);
    m_chosen.resize(group_count);

    for (int i = 0; i < group_count; i++)
    {
        m_chosen[i] = m_all_items[i].size();
    }
}

int MCKPSolver::pushItem(float value, float weight, int group)
{
    assert(group < m_all_items.size());

    m_all_items[group].push_back(std::make_pair(value, weight));
    return m_all_items[group].size() - 1;
}

void MCKPSolver::solve()
{
    // Dyer-Zemel Algorithm
    // Zemel, E. (1984). An O(n) algorithm for the linear multiple choice knapsack
    // problem and related problems. Information Processing Letters, 18(3), 123–128.
    // doi:10.1016/0020-0190(84)90014-0 
    
    std::vector<std::vector<int> > all_items;
    std::vector<bool> fathom;
    all_items.resize(m_all_items.size());
    fathom.resize(m_all_items.size());

    for (int i = 0; i < all_items.size(); i++)
    {
        all_items[i].resize(m_all_items[i].size());
        for (int j = 0; j < all_items[i].size(); j++)
        {
            all_items[i][j] = j;
        }
    }
    float capacity = m_capacity;

    while (1)
    {
        for (int i = 0; i < all_items.size(); i++)
        {
        	int j = 0;
            while (j + 1 < all_items[i].size())
            {
                int ia = j, ib = j + 1;
                int a = all_items[i][ia], b = all_items[i][ib];
                if (m_all_items[i][a].second > m_all_items[i][b].second
                || (m_all_items[i][a].second == m_all_items[i][b].second
                && m_all_items[i][a].first < m_all_items[i][b].first))
                {
                    std::swap(a, b);
                    std::swap(all_items[i][ia], all_items[i][ib]);
                    std::swap(ia, ib);
                }
                if (m_all_items[i][a].first >= m_all_items[i][b].first)
                {
                    all_items[i].erase(all_items[i].begin() + ib);
                }
                else j += 2;
            }
        }
        
        for (int i = 0; i < all_items.size(); i++)
        {
            if (all_items[i].size() == 1 && !fathom[i])
            {
            	fathom[i] = true;
                m_chosen[i] = all_items[i][0];
                capacity -= m_all_items[i][all_items[i][0]].second;
            }
        }
        std::vector<float> all_slopes;

        for (int i = 0; i < all_items.size(); i++)
        {
            for (int j = 0; j + 1 < all_items[i].size(); j += 2)
            {
                int ia = j, ib = j + 1;
                int a = all_items[i][ia], b = all_items[i][ib];
                all_slopes.push_back((m_all_items[i][a].first - m_all_items[i][b].first)
                                   / (m_all_items[i][a].second - m_all_items[i][b].second));
            }
        }
        if (all_slopes.empty())
        {
        	break;
        }

        std::nth_element(all_slopes.begin(), all_slopes.begin() + all_slopes.size() / 2, all_slopes.end());

        float alpha = all_slopes[all_slopes.size() / 2];

        float min_sum = 0, max_sum = 0;

        for (int i = 0; i < all_items.size(); i++)
        {
            if (all_items[i].size() == 1)
            {
                continue;
            }

            float max_intercept = -1e9;
            float min_weight = 1e9;
            float max_weight = -1e9;

            for (int j = 0; j < all_items[i].size(); j++)
            {
                int a = all_items[i][j];
                float intercept = m_all_items[i][a].first - alpha * m_all_items[i][a].second;

                if (intercept > max_intercept + Epsilon)
                {
                    max_intercept = intercept;
                    min_weight = max_weight = m_all_items[i][a].second;
                    m_chosen[i] = a;
                }
                else if (intercept >= max_intercept - Epsilon)
                {
                    float w = m_all_items[i][a].second;
                    if (w > max_weight)
                    {
                        max_weight = w;
                    }
                    else if (w < min_weight)
                    {
                        min_weight = w;
                        m_chosen[i] = a;
                    }
                }
            }
            min_sum += min_weight;
            max_sum += max_weight;
        }

        if (capacity < min_sum - Epsilon)
        {
            for (int i = 0; i < all_items.size(); i++)
            {
                for (int j = 0; j + 1 < all_items[i].size(); j += 2)
                {
                    int ia = j, ib = j + 1;
                    int a = all_items[i][ia], b = all_items[i][ib];
                    float slope = (m_all_items[i][a].first - m_all_items[i][b].first)
                                / (m_all_items[i][a].second - m_all_items[i][b].second);
                    if (alpha >= slope)
                    {
                        all_items[i].erase(all_items[i].begin() + ib);
                    }
                }
            }
        }
        else if (capacity > max_sum + Epsilon)
        {
            for (int i = 0; i < all_items.size(); i++)
            {
                for (int j = 0; j + 1 < all_items[i].size(); j += 2)
                {
                    int ia = j, ib = j + 1;
                    int a = all_items[i][ia], b = all_items[i][ib];
                    float slope = (m_all_items[i][a].first - m_all_items[i][b].first)
                                / (m_all_items[i][a].second - m_all_items[i][b].second);
                    if (alpha <= slope)
                    {
                        all_items[i].erase(all_items[i].begin() + ia);
                    }
                }
            }
        }
        else break;
    }
}