/**
 * ---
 *
 * # ItemInput
 *
 * This is an item type where a user can type in information,
 * the information is persisted in the item and can be gotten later by
 * using `item->value`
 */

#ifndef ItemInput_H
#define ItemInput_H

#include "MenuItem.h"
#include <utils/utils.h>

/**
 * @brief Item that allows user to input string information.
 *
 * ┌────────────────────────────┐
 * │ > T E X T : V A L U E      │
 * └────────────────────────────┘
 *
 * Additionally to `text` this item has string `value`.
 * Has internal `edit` state.
 * Value area is scrollable, see `view`.
 */
class ItemInput : public MenuItem {
  protected:
    /**
     * @brief String value of item.
     */
    char* value;
    /**
     * @brief The index of first visible character.
     *
     *          visible area
     *        ┌───────────────┐
     * X X X X│X X X X █ X X X│X X
     *        └───────────────┘
     *   view--^
     *
     * Is moved when `cursor` crosses the borders of visible area.
     * When length of `value` < `viewSize` this index should be 0.
     */
    uint8_t view = 0;
    /**
     * @brief Current index of the char to be edited.
     *
     *          visible area
     *        ┌───────────────┐
     * X X X X│X X X X █ X X X│X X
     *        └───────────────┘
     *         cursor--^
     *
     * When `left` or `right` then this position will be moved over the string accordingly.
     * When `type` then new character will be appended on this position.
     * When `backspace` then one character before will be removed.
     * Always in range [`view`, `view` + `viewSize` - 1].
     */
    uint8_t cursor;
    /**
     * The call back that will be executed when edit will be finished.
     * First parameter will be a `value` string.
     */
    fptrStr callback;
    /**
     * @brief The number of visible characters.
     *
     *          visible area
     *        ┌───────────────┐
     * X X X X│X X X X █ X X X│X X
     *        ├───────────────┤
     *        │<── viewSize ─>│
     *
     * Effectively const, but initialized lately when display is injected.
     */
    inline uint8_t getViewSize(DisplayInterface* display) {
        return display->getMaxCols() - (strlen(text) + 2) - 1;
    };

    uint8_t constrainBlinkerPosition(DisplayInterface* display, uint8_t blinkerPosition) {
        uint8_t maxCols = display->getMaxCols();
        //
        // calculate lower and upper bound
        //
        uint8_t lb = strlen(text) + 2;
        uint8_t ub = lb + strlen(value);
        ub = constrain(ub, lb, maxCols - 2);
        //
        // set cursor position
        //
        return constrain(blinkerPosition, lb, ub);
    }

  public:
    /**
     * Construct a new ItemInput object with an initial value.
     *
     * @param text The text to display for the item.
     * @param value The initial value for the input.
     * @param callback A reference to the callback function to be invoked when
     * the input is submitted.
     */
    ItemInput(const char* text, char* value, fptrStr callback)
        : MenuItem(text), value(value), callback(callback) {}
    /**
     * Construct a new ItemInput object with no initial value.
     *
     * @param text The text to display for the item.
     * @param callback A reference to the callback function to be invoked when
     * the input is submitted.
     */
    ItemInput(const char* text, fptrStr callback)
        : ItemInput(text, (char*)"", callback) {}
    /**
     * Get the current input value for this item.
     *
     * @return The current input value as a string.
     */
    char* getValue() { return value; }
    /**
     * Set the input value for this item.
     *
     * @param value The new input value.
     */
    void setValue(char* value) { this->value = value; }
    /**
     * Get the callback function for this item.
     *
     * @return The function pointer to the callback function.
     */
    fptrStr getCallbackStr() { return callback; }

  protected:
    void draw(DisplayInterface* display, uint8_t row) override {
        uint8_t maxCols = display->getMaxCols();
        static char* buf = new char[maxCols];
        substring(value, view, getViewSize(display), buf);
        display->drawItem(row, text, ':', buf);
    }
    bool process(Context& context) override {
        unsigned char c = context.command;
        DisplayInterface* display = context.display;
        if (isprint(c)) {
            return typeChar(context);
        }
        switch (c) {
            case ENTER: return enter(context);
            case BACK: return back(context);
            case UP: return display->getEditModeEnabled();
            case DOWN: return display->getEditModeEnabled();
            case LEFT: return left(context);
            case RIGHT: return right(context);
            case BACKSPACE: return backspace(context);
            case CLEAR: return clear(context);
            default: return false;
        }
    }
    bool enter(Context& context) {
        DisplayInterface* display = context.display;
        if (display->getEditModeEnabled()) {
            return false;
        }
        // Move cursor to the latest index
        uint8_t length = strlen(value);
        cursor = length;
        // Move view if needed
        uint8_t viewSize = getViewSize(display);
        if (cursor > viewSize) {
            view = length - (viewSize - 1);
        }
        // Redraw
        MenuItem::draw(display);
        display->setEditModeEnabled(true);
        display->resetBlinker(constrainBlinkerPosition(display, strlen(text) + 2 + cursor - view));
        display->drawBlinker();
        printLog(F("ItemInput::enterEditMode"), value);
        return true;
    };
    bool back(Context& context) {
        DisplayInterface* display = context.display;
        if (!display->getEditModeEnabled()) {
            return false;
        }
        display->clearBlinker();
        display->setEditModeEnabled(false);
        // Move view to 0 and redraw before exit
        cursor = 0;
        view = 0;
        MenuItem::draw(display);
        if (callback != NULL) {
            callback(value);
        }
        printLog(F("ItemInput::exitEditMode"), value);
        return true;
    };
    bool left(Context& context) {
        DisplayInterface* display = context.display;
        if (!display->getEditModeEnabled()) {
            return false;
        }
        if (cursor == 0) {
            return true;
        }
        cursor--;
        if (cursor < view) {
            view--;
            MenuItem::draw(display);
        }
        display->resetBlinker(constrainBlinkerPosition(display, display->getBlinkerPosition() - 1));
        // Log
        printLog(F("ItemInput::left"), value);
        return true;
    };
    bool right(Context& context) {
        DisplayInterface* display = context.display;
        if (!display->getEditModeEnabled()) {
            return false;
        }
        if (cursor == strlen(value)) {
            return true;
        }
        cursor++;
        uint8_t viewSize = getViewSize(display);
        if (cursor > (view + viewSize - 1)) {
            view++;
            MenuItem::draw(display);
        }
        display->resetBlinker(constrainBlinkerPosition(display, display->getBlinkerPosition() + 1));
        // Log
        printLog(F("ItemInput::right"), value);
        return true;
    };
    /**
     * Execute a "backspace cmd" on menu
     *
     * *NB: Works only for `ItemInput` type*
     *
     * Removes the character at the current cursor position.
     */
    bool backspace(Context& context) {
        DisplayInterface* display = context.display;
        if (!display->getEditModeEnabled()) {
            return false;
        }
        if (strlen(value) == 0 || cursor == 0) {
            return true;
        }
        remove(value, cursor - 1, 1);
        printLog(F("ItemInput::backspace"), value);
        cursor--;
        if (cursor < view) {
            view--;
        }
        MenuItem::draw(display);
        display->resetBlinker(constrainBlinkerPosition(display, display->getBlinkerPosition() - 1));
        return true;
    }
    /**
     * Display text at the cursor position
     * used for `Input` type menu items
     * @param character character to append
     */
    bool typeChar(Context& context) {
        DisplayInterface* display = context.display;
        const char character = context.command;
        if (!display->getEditModeEnabled()) {
            return false;
        }
        uint8_t length = strlen(value);
        if (cursor < length) {
            char start[length];
            char end[length];
            char* joined = new char[length + 2];
            substring(value, 0, cursor, start);
            substring(value, cursor, length - cursor, end);
            concat(start, character, end, joined);
            value = joined;
        } else {
            char* buf = new char[length + 2];
            concat(value, character, buf);
            value = buf;
        }
        cursor++;
        uint8_t viewSize = getViewSize(display);
        if (cursor > (view + viewSize - 1)) {
            view++;
        }
        MenuItem::draw(display);
        display->resetBlinker(constrainBlinkerPosition(display, display->getBlinkerPosition() + 1));
        // Log
        printLog(F("ItemInput::typeChar"), character);
        return true;
    }
    /**
     * Clear the value of the input field
     */
    bool clear(Context& context) {
        DisplayInterface* display = context.display;
        if (!display->getEditModeEnabled()) {
            return false;
        }
        //
        // set the value
        //
        value = (char*)"";
        // Log
        printLog(F("ItemInput::clear"), value);
        //
        // update blinker position
        //
        MenuItem::draw(display);
        display->resetBlinker(constrainBlinkerPosition(display, strlen(text) + 2));
        return true;
    }
};

#define ITEM_INPUT(...) (new ItemInput(__VA_ARGS__))

#endif  // ITEM_INPUT_H
