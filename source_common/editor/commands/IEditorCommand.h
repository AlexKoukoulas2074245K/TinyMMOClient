///------------------------------------------------------------------------------------------------
///  IEditorCommand.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 14/05/2024.
///------------------------------------------------------------------------------------------------

#ifndef IEditorCommand_h
#define IEditorCommand_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

class IEditorCommand
{
public:
    IEditorCommand() = default;
    virtual ~IEditorCommand() = default;
    IEditorCommand(const IEditorCommand&) = delete;
    const IEditorCommand& operator = (const IEditorCommand&) = delete;
    
    virtual void VExecute() = 0;
    virtual void VUndo() = 0;
    virtual bool VIsNoOp() const = 0;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* IEditorCommand_h */
