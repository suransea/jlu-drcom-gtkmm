//
// Created by sea on 12/23/18.
//

#include "preferences_dialog.h"

#include <giomm.h>

#include "../network/mac_address.h"
#include "drcomclient/config.h"
#include "singleton.h"

namespace DrcomClient {

PreferencesDialog::PreferencesDialog(BaseObjectType *object,
                                     const Glib::RefPtr<Gtk::Builder> &builder)
    : Gtk::Dialog(object) {
  builder->get_widget("auto_min_button", auto_min_button_);
  builder->get_widget("server_ip_entry", server_ip_entry_);
  builder->get_widget("mac_box", mac_box_);
  builder->get_widget("save_button", save_button_);
  builder->get_widget("cancel_button", cancel_button_);
  init_control();
  init_signal();
}

PreferencesDialog::~PreferencesDialog() {
  delete auto_min_button_;
  delete server_ip_entry_;
  delete mac_box_;
  delete save_button_;
  delete cancel_button_;
}

void PreferencesDialog::init_signal() {
  save_button_->signal_clicked().connect([this] { on_save_button_clicked(); });
  cancel_button_->signal_clicked().connect([this] { on_cancel_button_clicked(); });
}

void PreferencesDialog::init_control() {
  auto config = Singleton<Config>::instance();
  auto_min_button_->set_active(config->auto_min());
  server_ip_entry_->set_text(config->server_ip());
  mac_box_->append(config->mac_address());
  auto &&macs = MacAddress::all();
  for (auto &mac : macs) {
    if (mac.to_string() == config->mac_address())
      continue;
    mac_box_->append(mac.to_string());
  }
  mac_box_->set_active(0);
}

void PreferencesDialog::on_save_button_clicked() {
  auto config = Singleton<Config>::instance();
  config->set_auto_min(auto_min_button_->get_active());
  config->set_mac_address(mac_box_->get_active_text());
  config->set_server_ip(server_ip_entry_->get_text());
  config->save();
  close();
}

void PreferencesDialog::on_cancel_button_clicked() { close(); }

} // namespace DrcomClient