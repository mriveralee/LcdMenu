/**
 * ---
 *
 * # ItemCommand
 *
 * This item type indicates that the current item is a **command**.
 * When `enter()` is invoked, the command *(callback)* bound to this item is
 * invoked.
 */

#ifndef ItemCommand_H
#define ItemCommand_H

// Include the header file for the base class.
#include "MenuItem.h"

// Declare a class for menu items that represent commands.
class ItemCommand : public MenuItem {
  private:
    // Declare a function pointer for the command callback.
    fptr callback;

  public:
    /**
     * Construct a new ItemCommand object.
     *
     * @param text The text of the item.
     * @param callback A reference to the callback function to be invoked when
     * the item is entered.
     */
    ItemCommand(const char* text, fptr callback) : MenuItem(text), callback(callback) {}

    /**
     * Get the callback function for this item.
     *
     * @return The function pointer to the callback function.
     */
    fptr getCallback() { return callback; }

    /**
     * Set the callback function for this item.
     *
     * @param callback A reference to the new callback function to be invoked
     * when the item is entered.
     */
    void setCallBack(fptr callback) { this->callback = callback; };

  protected:
    bool process(Context& context) override {
        switch (context.command) {
            case ENTER: return enter(context);
            default: return false;
        }
    }
    bool enter(Context& context) {
        if (callback != NULL) {
            callback();
        }
        printLog(F("ItemCommand::enter"), text);
        return true;
    };
};

#define ITEM_COMMAND(...) (new ItemCommand(__VA_ARGS__))

#endif  // ITEM_COMMAND_H
