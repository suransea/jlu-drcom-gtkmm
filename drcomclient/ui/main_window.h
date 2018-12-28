//
// Created by sea on 12/22/18.
//

#ifndef JLU_DRCOM_MAIN_WINDOW_H
#define JLU_DRCOM_MAIN_WINDOW_H


#include <gtkmm/window.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/checkbutton.h>

#include "../network/drcom.h"

namespace drcomclient {

class MainWindow : public Gtk::Window {
public:
  MainWindow(BaseObjectType *object, const Glib::RefPtr<Gtk::Builder> &builder);

  MainWindow(const MainWindow &) = delete;

  ~MainWindow() override;

protected:
  void login();

  void init_control();

  void init_css();

  void init_signal();

  //Signal handlers:
  void on_login_button_clicked();

  void on_login_done(bool success, const std::string &msg);

  void on_logout_done(bool success, const std::string &msg);

  void on_alive_abort(bool, const std::string &msg);

  //Member widgets:
  Gtk::Button *login_button_ = nullptr;
  Gtk::Entry *user_entry_ = nullptr;
  Gtk::Entry *passwd_entry_ = nullptr;
  Gtk::CheckButton *remember_button_ = nullptr;
  Gtk::CheckButton *auto_button_ = nullptr;

  Drcom drcom_;
};

} //namespace drcomclient

#endif //JLU_DRCOM_MAIN_WINDOW_H
