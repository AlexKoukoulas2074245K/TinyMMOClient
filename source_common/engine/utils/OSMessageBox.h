///------------------------------------------------------------------------------------------------
///  OSMessageBox.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 19/09/2023.
///-----------------------------------------------------------------------------------------------

#ifndef OSMessageBox_h
#define OSMessageBox_h

///-----------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <SDL_messagebox.h>
#include <string>

///-----------------------------------------------------------------------------------------------

namespace ospopups
{

///-----------------------------------------------------------------------------------------------
/// Different types of message box types available.
enum class MessageBoxType
{
    INFO    = SDL_MessageBoxFlags::SDL_MESSAGEBOX_INFORMATION,
    WARNING = SDL_MessageBoxFlags::SDL_MESSAGEBOX_WARNING,   
    ERROR   = SDL_MessageBoxFlags::SDL_MESSAGEBOX_ERROR,
};

///-----------------------------------------------------------------------------------------------
/// Show an SDL driven message box with a custom title, description, and of 
/// a particular type \see MessageBoxType.
/// @param[in] messageBoxType the type of message box to show.
/// @param[in] title the title of the window that will be displayed.
/// @param[in] description the description in the main body of the window that will be displayed.
inline void ShowInfoMessageBox
(
    const MessageBoxType messageBoxType,
    const std::string& title,
    const std::string description = ""
)
{
    SDL_ShowSimpleMessageBox
    (
        static_cast<SDL_MessageBoxFlags>(messageBoxType),
        title.c_str(),
        description.empty() ? title.c_str() : description.c_str(),
        nullptr
    );
}

///-----------------------------------------------------------------------------------------------
/// Show an SDL driven message box with a custom title, description, and of
/// a particular type. Also includes Okay/Cancel buttons
/// @param[in] messageBoxType the type of message box to show.
/// @param[in] title the title of the window that will be displayed.
/// @param[in] description the description in the main body of the window that will be displayed.
/// @returns 0 for Okay pressed / 1 for Cancel pressed
inline int ShowOkayCancelMessageBox
(
    const MessageBoxType messageBoxType,
    const std::string& title,
    const std::string description = ""
)
{
    int selectedButtonId = 0;
    SDL_MessageBoxData messageBoxData = {};
    messageBoxData.flags = static_cast<SDL_MessageBoxFlags>(messageBoxType);
    messageBoxData.window = &CoreSystemsEngine::GetInstance().GetContextWindow();
    messageBoxData.title = title.c_str();
    messageBoxData.message = description.empty() ? title.c_str() : description.c_str();
    messageBoxData.numbuttons = 2;
    
    SDL_MessageBoxButtonData buttonData[2];
    buttonData[0] = { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel" };
    buttonData[1] = { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Okay" };
    
    messageBoxData.buttons = buttonData;
    
    SDL_ShowMessageBox
    (
        &messageBoxData,
        &selectedButtonId
    );
    
    return selectedButtonId;
}

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------

#endif /* OSMessageBox_h */
