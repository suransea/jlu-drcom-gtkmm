//
// Created by sea on 12/22/18.
//

#ifndef JLU_DRCOM_APPLICATION_H
#define JLU_DRCOM_APPLICATION_H

#include <gtkmm/application.h>

#include "ui/main_window.h"

namespace drcomclient {

class Application : public Gtk::Application {
public:
  Application();

  Application(const Application &) = delete;

protected:
  void on_startup() override;

  void on_activate() override;

  void on_preferences_menu_acted();

  void on_quit_menu_acted();

  void on_about_menu_acted();
};

} //namespace drcomclient

#endif //JLU_DRCOM_APPLICATION_H
