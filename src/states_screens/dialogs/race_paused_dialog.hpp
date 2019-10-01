//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2015 Marianne Gagnon
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

#ifndef HEADER_RACE_PAUSED_DIALOG_HPP
#define HEADER_RACE_PAUSED_DIALOG_HPP

#include "guiengine/modaldialog.hpp"
#include "guiengine/widgets/text_box_widget.hpp"
#include "utils/cpp2011.hpp"

namespace GUIEngine
{
    class RibbonWidget;
    class TextBoxWidget;
}

/**
 * \brief Dialog shown when the race is paused
 * \ingroup states_screens
 */
class RacePausedDialog : public GUIEngine::ModalDialog,
                         public GUIEngine::ITextBoxWidgetListener
{
private:
    bool m_self_destroy;

    GUIEngine::TextBoxWidget* m_text_box;

    virtual void onTextUpdated() OVERRIDE {}
    virtual bool onEnterPressed(const irr::core::stringw& text) OVERRIDE;

protected:
    virtual void loadedFromFile() OVERRIDE;

public:
    /**
     * Creates a modal dialog with given percentage of screen width and height
     */
    RacePausedDialog(const float percentWidth, const float percentHeight);
    virtual ~RacePausedDialog();
    virtual void onEnterPressedInternal() OVERRIDE;
    GUIEngine::EventPropagation processEvent(const std::string& event_source)
        OVERRIDE;
    virtual void beforeAddingWidgets() OVERRIDE;
    // ------------------------------------------------------------------------
    virtual void onUpdate(float dt) OVERRIDE
    {
        // It's unsafe to delete from inside the event handler so we do it here
        if (m_self_destroy)
        {
            ModalDialog::dismiss();
            return;
        }
    }
};

#endif
