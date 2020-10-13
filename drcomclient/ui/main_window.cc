//
// Created by sea on 12/22/18.
//

#include "main_window.h"

#include <gtkmm/cssprovider.h>
#include <gtkmm/messagedialog.h>

#include "drcomclient/config.h"
#include "singleton.h"

namespace DrcomClient {

namespace {

enum class LoginButtonStatus { LOGIN, LOGOUT, LOGGING };

LoginButtonStatus button_status = LoginButtonStatus::LOGIN;

} // namespace

MainWindow::MainWindow(BaseObjectType *object,
                       const Glib::RefPtr<Gtk::Builder> &builder)
    : Gtk::Window(object) {
  builder->get_widget("login_button", login_button_);
  builder->get_widget("user_entry", user_entry_);
  builder->get_widget("passwd_entry", passwd_entry_);
  builder->get_widget("remember_button", remember_button_);
  builder->get_widget("auto_button", auto_button_);
  init_control();
  init_css();
  init_signal();
}

void MainWindow::on_login_button_clicked() {
  switch (button_status) {
  case LoginButtonStatus::LOGIN:
    login();
    break;
  case LoginButtonStatus::LOGOUT:
    drcom_.do_logout();
    break;
  case LoginButtonStatus::LOGGING:
    drcom_.cancel();
    on_logout_done(true, "");
    break;
  }
}

void MainWindow::init_control() {
  auto config = Singleton<Config>::instance();
  remember_button_->set_active(config->remember_me());
  auto_button_->set_active(config->auto_login());
  user_entry_->set_text(config->user());
  passwd_entry_->set_text(config->password());
  if (config->auto_login()) {
    on_login_button_clicked();
  }
}

void MainWindow::init_css() {
  auto provider = Gtk::CssProvider::create();
  provider->load_from_resource("/com/sea/drcomclient/main.css");
  auto screen = Gdk::Screen::get_default();
  auto context = get_style_context();
  context->add_provider_for_screen(screen, provider,
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void MainWindow::init_signal() {
  auto login_slot = [this] { on_login_button_clicked(); };
  login_button_->signal_clicked().connect(login_slot);
  passwd_entry_->signal_activate().connect(login_slot);
  user_entry_->signal_activate().connect(login_slot);
  auto_button_->signal_toggled().connect([this] {
    if (auto_button_->get_active()) {
      remember_button_->set_active(true);
    }
  });
  remember_button_->signal_toggled().connect([this] {
    if (!remember_button_->get_active()) {
      auto_button_->set_active(false);
    }
  });
  signal_hide().connect([this] {
    drcom_.cancel();
    close();
  });
  drcom_.signal_login().connect(
      sigc::mem_fun(this, &MainWindow::on_login_done));
  drcom_.signal_logout().connect(
      sigc::mem_fun(this, &MainWindow::on_logout_done));
  drcom_.signal_abort().connect(
      sigc::mem_fun(this, &MainWindow::on_alive_abort));
}

MainWindow::~MainWindow() {
  delete login_button_;
  delete user_entry_;
  delete passwd_entry_;
  delete remember_button_;
  delete auto_button_;
}

void MainWindow::login() {
  auto &&user = user_entry_->get_text();
  auto &&password = passwd_entry_->get_text();
  if (user.empty() || password.empty()) {
    login_button_->set_label("Please Fill the Blank");
    return;
  }
  login_button_->set_label("Logging");
  button_status = LoginButtonStatus::LOGGING;
  auto config = Singleton<Config>::instance();
  config->set_auto_login(auto_button_->get_active());
  config->set_remember_me(remember_button_->get_active());
  if (config->remember_me()) {
    config->set_user(user);
    config->set_password(password);
  } else {
    config->set_user("");
    config->set_password("");
  }
  config->save();
  drcom_.do_login(user, password);
}

void MainWindow::on_login_done(bool success, const std::string &msg) {
  if (!success) {
    login_button_->set_label("Login");
    button_status = LoginButtonStatus::LOGIN;
    Gtk::MessageDialog dialog(*this, msg, false, Gtk::MESSAGE_WARNING,
                              Gtk::BUTTONS_OK);
    dialog.run();
    return;
  }
  login_button_->set_label("Logout");
  button_status = LoginButtonStatus::LOGOUT;
  login_button_->set_sensitive(true);
  user_entry_->set_sensitive(false);
  passwd_entry_->set_sensitive(false);
  remember_button_->set_sensitive(false);
  auto_button_->set_sensitive(false);
  auto config = Singleton<Config>::instance();
  if (config->auto_min())
    iconify();
}

void MainWindow::on_logout_done(bool success, const std::string &msg) {
  if (!success) {
    Gtk::MessageDialog dialog(*this, msg, false, Gtk::MESSAGE_WARNING,
                              Gtk::BUTTONS_OK);
    dialog.run();
  }
  login_button_->set_label("Login");
  button_status = LoginButtonStatus::LOGIN;
  user_entry_->set_sensitive(true);
  passwd_entry_->set_sensitive(true);
  remember_button_->set_sensitive(true);
  auto_button_->set_sensitive(true);
}

void MainWindow::on_alive_abort(bool, const std::string &msg) {
  Gtk::MessageDialog dialog(*this, msg, false, Gtk::MESSAGE_WARNING,
                            Gtk::BUTTONS_OK);
  dialog.run();
  on_logout_done(true, "");
}

} // namespace DrcomClient