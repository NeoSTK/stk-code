//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013 Glenn De Jonghe
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

#include "states_screens/dialogs/login_dialog.hpp"

#include <IGUIEnvironment.h>

#include "audio/sfx_manager.hpp"
#include "config/player.hpp"
#include "guiengine/engine.hpp"
#include "states_screens/state_manager.hpp"
#include "utils/translation.hpp"
#include "utils/string_utils.hpp"
#include "states_screens/dialogs/registration_dialog.hpp"
#include "online/messages.hpp"


using namespace GUIEngine;
using namespace irr;
using namespace irr::gui;
using namespace Online;

// -----------------------------------------------------------------------------

LoginDialog::LoginDialog(const Message message_type) :
        ModalDialog(0.8f,0.9f)
{
    m_self_destroy = false;
    m_open_registration_dialog = false;
    m_signing_in = false;
    loadFromFile("online/login_dialog.stkgui");

    m_message_widget = getWidget<LabelWidget>("message");
    assert(m_message_widget != NULL);
    irr::core::stringw message;
    if (message_type == Normal)
        message =  _("Fill in your username and password. ");
    else if (message_type == Signing_In_Required)
        message =  _("You need to sign in to be able to use this feature. ");
    else if (message_type == Registration_Required)
        message =  _("You need to be a registered user to enjoy this feature! "
                  "If you do not have an account yet, you can sign up using the register icon at the bottom.");
    else
        message = "";
    if (message_type == Normal || message_type == Signing_In_Required)
        message += _("If you do not have an account yet, you can choose to sign in as a guest "
                  "or press the register icon at the bottom to gain access to all the features!");
    m_message_widget->setText(message, false);

    m_username_widget = getWidget<TextBoxWidget>("username");
    assert(m_username_widget != NULL);
    m_username_widget->setFocusForPlayer(PLAYER_ID_GAME_MASTER);

    m_password_widget = getWidget<TextBoxWidget>("password");
    assert(m_password_widget != NULL);
    m_password_widget->setPasswordBox(true,L'*');

    m_remember_widget = getWidget<CheckBoxWidget>("remember");
    assert(m_remember_widget != NULL);
    m_remember_widget->setState(false);

    m_info_widget = getWidget<LabelWidget>("info");
    assert(m_info_widget != NULL);

    m_options_widget = getWidget<RibbonWidget>("options");
    assert(m_options_widget != NULL);
    m_sign_in_widget = getWidget<IconButtonWidget>("sign_in");
    assert(m_sign_in_widget != NULL);
    m_recovery_widget = getWidget<IconButtonWidget>("recovery");
    assert(m_recovery_widget != NULL);
    m_register_widget = getWidget<IconButtonWidget>("register");
    assert(m_register_widget != NULL);
    m_as_guest_widget = getWidget<IconButtonWidget>("as_guest");
    assert(m_as_guest_widget != NULL);
    m_cancel_widget = getWidget<IconButtonWidget>("cancel");
    assert(m_cancel_widget != NULL);


}

// -----------------------------------------------------------------------------

LoginDialog::~LoginDialog()
{
}



// -----------------------------------------------------------------------------
void LoginDialog::login()
{
    const stringw username = m_username_widget->getText().trim();
    const stringw password = m_password_widget->getText().trim();
    if (username.size() < 4 || username.size() > 30 || password.size() < 8 || password.size() > 30)
    {
        sfx_manager->quickSound("anvil");
        m_info_widget->setErrorColor();
        m_info_widget->setText(_("Username and/or password invalid."), false);
    }
    else
    {
        m_options_widget->setDeactivated();
        Online::CurrentUser::acquire()->requestSignIn(username,password, m_remember_widget->getState());
        Online::CurrentUser::release();
        m_signing_in = true;
    }
}

// -----------------------------------------------------------------------------
GUIEngine::EventPropagation LoginDialog::processEvent(const std::string& eventSource)
{

    if (eventSource == m_options_widget->m_properties[PROP_ID])
    {
        const std::string& selection = m_options_widget->getSelectionIDString(PLAYER_ID_GAME_MASTER);
        if (selection == m_cancel_widget->m_properties[PROP_ID])
        {
            m_self_destroy = true;
            return GUIEngine::EVENT_BLOCK;
        }
        else if(selection == m_sign_in_widget->m_properties[PROP_ID])
        {
            login();
            return GUIEngine::EVENT_BLOCK;
        }
        else if(selection == m_register_widget->m_properties[PROP_ID])
        {
            m_open_registration_dialog = true;
            return GUIEngine::EVENT_BLOCK;
        }
    }
    return GUIEngine::EVENT_LET;
}

// -----------------------------------------------------------------------------

void LoginDialog::onEnterPressedInternal()
{

    //If enter was pressed while none of the buttons was focused interpret as sign_in event
    const int playerID = PLAYER_ID_GAME_MASTER;
    if (GUIEngine::isFocusedForPlayer(m_options_widget, playerID))
        return;
    if (m_sign_in_widget->isActivated())
        login();
}

// -----------------------------------------------------------------------------

void LoginDialog::onUpdate(float dt)
{
    XMLRequest * sign_in_request = HTTPManager::get()->getXMLResponse(Request::RT_SIGN_IN);
    if(sign_in_request != NULL)
    {
        if(sign_in_request->isSuccess())
        {
            m_self_destroy = true;
        }
        else
        {
            sfx_manager->quickSound( "anvil" );
            m_info_widget->setErrorColor();
            m_info_widget->setText(sign_in_request->getInfo(), false);
        }
        delete sign_in_request;
        m_signing_in = false;
        m_options_widget->setActivated();
    }
    //If we want to open the registration dialog, we need to close this one first
    m_open_registration_dialog && (m_self_destroy = true);

    // It's unsafe to delete from inside the event handler so we do it here
    if (m_self_destroy)
    {
        ModalDialog::dismiss();
        if (m_open_registration_dialog)
            new RegistrationDialog();
        return;
    }

    if (m_signing_in)
    {
        m_info_widget->setDefaultColor();
        m_info_widget->setText(Online::Messages::signingIn(), false);
    }
}
