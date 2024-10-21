#ifndef ITEM_WIDGET_H
#define ITEM_WIDGET_H

#include "BaseItemManyWidgets.h"
#include "widget/BaseWidgetValue.h"

// Custom index_sequence implementation for C++11
template <size_t... Is>
struct index_sequence {};

template <size_t N, size_t... Is>
struct make_index_sequence : make_index_sequence<N - 1, N - 1, Is...> {};

template <size_t... Is>
struct make_index_sequence<0, Is...> {
    using type = index_sequence<Is...>;
};

// Updated ItemWidget class supporting zero or more widgets
template <typename... Ts>
class ItemWidget : public BaseItemManyWidgets {
  public:
    using CallbackType = void (*)(Ts...);

  protected:
    CallbackType callback = nullptr;

    void handleCommit() override {
        if (callback != nullptr) {
            invokeCallback(typename make_index_sequence<sizeof...(Ts)>::type{});
        }
    }

    template <size_t... Is>
    void invokeCallback(index_sequence<Is...>) {
        callback(static_cast<BaseWidgetValue<Ts>*>(widgets[Is])->getValue()...);
    }

  public:
    // Constructor for one or more widgets
    ItemWidget(const char* text, BaseWidgetValue<Ts>*... widgetPtrs, CallbackType callback = nullptr)
        : BaseItemManyWidgets(
              text,
              // clang-format off
              new BaseWidget* [sizeof...(Ts)] { widgetPtrs... },
              // clang-format on
              sizeof...(Ts)),
          callback(callback) {}

    void setValues(Ts... values) {
        setValuesImpl(typename make_index_sequence<sizeof...(Ts)>::type{}, values...);
    }

  private:
    template <size_t... Is>
    void setValuesImpl(index_sequence<Is...>, Ts... values) {
        // Using a dummy array to expand the parameter pack and call setValue on each widget
        int dummy[] = {(static_cast<BaseWidgetValue<Ts>*>(widgets[Is])->setValue(values), 0)...};
        static_cast<void>(dummy);  // Avoid unused variable warning
    }
};

template <typename... Ts>
/**
 * @brief Create an ItemWidget object
 *
 * @param text the text of the item
 * @param callback reference to callback function to call when the value of the item is changed
 * @param widgetPtrs pointers to the widgets associated with this item
 */
MenuItem* ITEM_WIDGET(const char* text, typename ItemWidget<Ts...>::CallbackType callback, BaseWidgetValue<Ts>*... widgetPtrs) {
    return new ItemWidget<Ts...>(text, widgetPtrs..., callback);
}

#endif  // ITEM_WIDGET_H