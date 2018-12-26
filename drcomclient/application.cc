//
// Created by sea on 12/22/18.
//

#include "application.h"

#include <gtkmm/builder.h>

#include "ui/about_dialog.h"
#include "ui/preferences_dialog.h"

namespace drcomclient {

Application::Application()
    : Gtk::Application("com.sea.drcomclient") {
}

void Application::on_preferences_menu_acted() {
  PreferencesDialog *dialog;
  auto builder = Gtk::Builder::create();
  builder->add_from_resource("/com/sea/drcomclient/preferences.glade");
  builder->get_widget_derived("pre_dialog", dialog);
  dialog->signal_hide().connect([dialog] { delete dialog; });
  dialog->show();
}

void Application::on_quit_menu_acted() {
  for (auto &it:get_windows()) {
    it->hide();
  }
  quit();
  exit(0);
}

void Application::on_about_menu_acted() {
  AboutDialog *dialog;
  auto builder = Gtk::Builder::create();
  builder->add_from_resource("/com/sea/drcomclient/about.glade");
  builder->get_widget_derived("about", dialog);
  dialog->signal_hide().connect([dialog] { delete dialog; });
  dialog->show();
}

void Application::on_startup() {
  Gtk::Application::on_startup();
  add_action("preferences", [&] { on_preferences_menu_acted(); });
  add_action("about", [&] { on_about_menu_acted(); });
  add_action("quit", [&] { on_quit_menu_acted(); });
  auto builder = Gtk::Builder::create();
  builder->add_from_resource("/com/sea/drcomclient/app_menu.glade");
  auto object = builder->get_object("app_menu");
  auto app_menu = Glib::RefPtr<Gio::Menu>::cast_dynamic(object);
  set_app_menu(app_menu);
}

void Application::on_activate() {
  Gtk::Application::on_activate();
  auto builder = Gtk::Builder::create();
  builder->add_from_resource("/com/sea/drcomclient/main.glade");
  MainWindow *window;
  builder->get_widget_derived("window", window);
  window->signal_hide().connect([window] { delete window; });
  add_window(*window);
  window->show();
}


} //namespace drcomclient